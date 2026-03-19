import cv2
import torch
import os
import urllib.request
from basicsr.archs.rrdbnet_arch import RRDBNet
from realesrgan import RealESRGANer

def main():
    # 1. Детектор железа
    # Если найдет NVIDIA с CUDA - будет использовать видеокарту. Иначе упадет на процессор.
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"[*] Используемое устройство: {device}")
    
    if device.type == 'cpu':
        print("[!] Внимание: CUDA не найдена. На AMD встройке обработка идет через процессор (будет медленно).")

    # 2. Инициализация архитектуры модели
    # RRDBNet - это классическая архитектура для Real-ESRGAN (x4 увеличение)
    print("[*] Настройка нейросети...")
    model = RRDBNet(num_in_ch=3, num_out_ch=3, num_feat=64, num_block=23, num_grow_ch=32, scale=4)
    
    # 3. Автоматическое скачивание весов (файла модели .pth), если его нет
    model_path = 'RealESRGAN_x4plus.pth'
    if not os.path.exists(model_path):
        print(f"[*] Скачиваю веса модели {model_path} (~67 MB)...")
        url = 'https://github.com/xinntao/Real-ESRGAN/releases/download/v0.1.0/RealESRGAN_x4plus.pth'
        urllib.request.urlretrieve(url, model_path)
        print("[*] Скачивание завершено!")

    # 4. Создание "Апскейлера"
    upsampler = RealESRGANer(
        scale=4,                  # Увеличение в 4 раза
        model_path=model_path,
        model=model,
        tile=0,                   # 0 = обрабатывать картинку целиком. Если не хватит VRAM на 3060, поставим 256 или 512
        tile_pad=10,
        pre_pad=0,
        half=(device.type == 'cuda'), # fp16 (половинная точность) работает только на CUDA, дает x2 скорости
        device=device
    )

    # 5. Чтение картинки
    input_path = 'input.jpg'
    output_path = 'output.png'

    if not os.path.exists(input_path):
        print(f"[!] Ошибка: Положите тестовое изображение {input_path} рядом со скриптом!")
        return

    print(f"[*] Читаем изображение {input_path}...")
    img = cv2.imread(input_path, cv2.IMREAD_UNCHANGED)

    # 6. Магия апскейла
    print("[*] Нейронка думает... (на CPU это может занять от 10 секунд до минуты)")
    # outscale=4 означает, что мы просим итоговое увеличение ровно в 4 раза
    output, _ = upsampler.enhance(img, outscale=4)

    # 7. Сохранение
    cv2.imwrite(output_path, output)
    print(f"[*] Готово! Увеличенная в 4 раза картинка сохранена в {output_path}")

if __name__ == '__main__':
    main()
