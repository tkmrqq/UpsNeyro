"""
upscaler.py — Python сторона пайплайна апскейла
"""

import argparse
import struct
import sys
import time
import os
import numpy as np

HEADER_SIZE = 64

STATE_IDLE      = 0
STATE_CPP_READY = 1
STATE_PY_READY  = 2
STATE_STOP      = 3

MODEL_WEIGHTS = {
    'realesr-animevideov3':    'weights/realesr-animevideov3.pth',
    'realesrgan-x4plus':       'weights/RealESRGAN_x4plus.pth',
    'realesrgan-x4plus-anime': 'weights/RealESRGAN_x4plus_anime_6B.pth',
}

# ─────────────────────────────────────────────────────────────────────────────

def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('--shm-key',  required=True)
    p.add_argument('--shm-size', type=int, required=True)
    p.add_argument('--model',    default='realesrgan-x4plus')
    p.add_argument('--scale',    type=int, default=4)
    p.add_argument('--device',   default='auto')
    return p.parse_args()


def load_model(model_name: str, scale: int, device_str: str):
    import torch
    from realesrgan import RealESRGANer

    model_path = MODEL_WEIGHTS.get(model_name)
    if not model_path:
        raise ValueError(f"Unknown model '{model_name}'. "
                         f"Available: {list(MODEL_WEIGHTS.keys())}")

    script_dir = os.path.dirname(os.path.abspath(__file__))
    model_path = os.path.join(script_dir, model_path)

    if not os.path.exists(model_path):
        raise FileNotFoundError(f"Model weights not found: {model_path}")

    if device_str == 'auto':
        device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    elif device_str == 'cuda':
        device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    else:
        device = torch.device('cpu')

    print(f"[upscaler.py] Device: {device}", flush=True)
    print(f"[upscaler.py] Model: {model_path}", flush=True)

    if 'animevideo' in model_name:
        from basicsr.archs.srvgg_arch import SRVGGNetCompact
        model = SRVGGNetCompact(
            num_in_ch=3, num_out_ch=3,
            num_feat=64, num_conv=16,
            upscale=scale, act_type='prelu'
        )
    elif 'anime' in model_name:
        from basicsr.archs.rrdbnet_arch import RRDBNet
        model = RRDBNet(num_in_ch=3, num_out_ch=3,
                        num_feat=64, num_block=6, num_grow_ch=32,
                        scale=scale)
    else:
        from basicsr.archs.rrdbnet_arch import RRDBNet
        model = RRDBNet(num_in_ch=3, num_out_ch=3,
                        num_feat=64, num_block=23, num_grow_ch=32,
                        scale=scale)

    upsampler = RealESRGANer(
        scale=scale,
        model_path=model_path,
        model=model,
        tile=512,
        tile_pad=10,
        pre_pad=0,
        half=(device.type == 'cuda'),
        device=device
    )
    return upsampler


# ─────────────────────────────────────────────────────────────────────────────
# Shared Memory — кроссплатформ
# ─────────────────────────────────────────────────────────────────────────────

class ShmWrapper:
    """Единый интерфейс для Windows и Linux."""

    def __init__(self):
        self._impl = None

    def open(self, key: str, size: int):
        if sys.platform == 'win32':
            from shm_windows import WinSharedMemory
            self._impl = WinSharedMemory(key, size)
            self._impl.open()
        else:
            import mmap
            shm_path = f'/dev/shm/{key}'
            if not os.path.exists(shm_path):
                raise RuntimeError(f"SHM not found: {shm_path}")
            fd = os.open(shm_path, os.O_RDWR)
            self._mm = mmap.mmap(fd, size)
            os.close(fd)
            self._impl = self  # self как Linux impl
            self._is_linux = True
        return self

    def close(self):
        if sys.platform == 'win32':
            if self._impl:
                self._impl.close()
        else:
            if hasattr(self, '_mm') and self._mm:
                self._mm.close()

    # ── Чтение/запись ─────────────────────────────────────────────────────────

    def get_state(self) -> int:
        if sys.platform == 'win32':
            return self._impl.read_byte(0)
        else:
            self._mm.seek(0)
            return struct.unpack('B', self._mm.read(1))[0]

    def set_state(self, state: int):
        if sys.platform == 'win32':
            self._impl.write_byte(0, state)
        else:
            self._mm.seek(0)
            self._mm.write(struct.pack('B', state))

    def read_header(self):
        if sys.platform == 'win32':
            data = self._impl.read_bytes(1, 16)
        else:
            self._mm.seek(1)
            data = self._mm.read(16)
        return struct.unpack('IIII', data)

    def read_input_frame(self, src_w: int, src_h: int) -> np.ndarray:
        size = src_w * src_h * 3
        if sys.platform == 'win32':
            data = self._impl.read_bytes(HEADER_SIZE, size)
        else:
            self._mm.seek(HEADER_SIZE)
            data = self._mm.read(size)
        return np.frombuffer(data, dtype=np.uint8).reshape(src_h, src_w, 3).copy()

    def write_output_frame(self, src_w: int, src_h: int, frame: np.ndarray):
        offset = HEADER_SIZE + src_w * src_h * 3
        raw = frame.tobytes()
        if sys.platform == 'win32':
            self._impl.write_bytes(offset, raw)
        else:
            self._mm.seek(offset)
            self._mm.write(raw)

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


# ─────────────────────────────────────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────────────────────────────────────

def main():
    args = parse_args()

    print(f"[upscaler.py] Starting: model={args.model} scale={args.scale} "
          f"device={args.device}", flush=True)

    try:
        upsampler = load_model(args.model, args.scale, args.device)
    except Exception as e:
        print(f"[upscaler.py] ERROR loading model: {e}", file=sys.stderr, flush=True)
        sys.exit(1)

    try:
        shm = ShmWrapper()
        shm.open(args.shm_key, args.shm_size)
    except Exception as e:
        print(f"[upscaler.py] ERROR opening SHM: {e}", file=sys.stderr, flush=True)
        sys.exit(1)

    print("READY", flush=True)
    print("[upscaler.py] Waiting for frames...", flush=True)

    import cv2
    frame_count = 0

    try:
        while True:
            state = shm.get_state()

            if state == STATE_STOP:
                print(f"[upscaler.py] STOP, processed {frame_count} frames",
                      flush=True)
                break

            if state != STATE_CPP_READY:
                time.sleep(0.001)
                continue

            src_w, src_h, dst_w, dst_h = shm.read_header()
            frame_rgb = shm.read_input_frame(src_w, src_h)

            try:
                frame_bgr  = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
                out_bgr, _ = upsampler.enhance(frame_bgr, outscale=args.scale)
                out_rgb    = cv2.cvtColor(out_bgr, cv2.COLOR_BGR2RGB)
            except Exception as e:
                print(f"[upscaler.py] Frame {frame_count} error: {e}",
                      file=sys.stderr, flush=True)
                out_rgb = cv2.resize(frame_rgb, (dst_w, dst_h),
                                     interpolation=cv2.INTER_LANCZOS4)

            shm.write_output_frame(src_w, src_h, out_rgb)
            shm.set_state(STATE_PY_READY)
            frame_count += 1

            if frame_count % 10 == 0:
                print(f"[upscaler.py] Processed {frame_count} frames", flush=True)

    finally:
        shm.close()

    print("[upscaler.py] Done", flush=True)


if __name__ == '__main__':
    main()