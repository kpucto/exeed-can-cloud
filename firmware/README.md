# EXEED CAN ROUTER v2.0

Единая прошивка ESP32-S3: приём CAN в аппаратном режиме TWAI LISTEN ONLY, локальная веб-панель, инвентаризация CAN ID и передача принятых кадров в Cloudflare Worker.

## Параметры

- ESP32-S3 DevKitC-1 compatible
- SN65HVD230 / WCMCU-230 CAN transceiver
- GPIO4 = TWAI TX -> TXD трансивера
- GPIO5 = TWAI RX <- RXD трансивера
- OBD-II pin 6 = CAN-H, pin 14 = CAN-L
- 500 kbit/s, accept-all filter, TWAI LISTEN ONLY
- Cloudflare WSS: wss://exeed-can-cloud.kpucto.workers.dev/device

В прошивке отсутствуют вызовы передачи CAN. TX queue отключена, TWAI работает в LISTEN ONLY.

## Первичная настройка Wi-Fi

1. Прошейте проект через PlatformIO.
2. Подключитесь к точке Wi-Fi EXEED_CAN_ROUTER.
3. Пароль точки настройки: exeed123.
4. Откройте http://192.168.4.1.
5. Введите SSID и пароль автомобильного модема (например, SSID J7) и сохраните.
6. Плата перезапустится и подключится к модему. Данные сохраняются в NVS.

## Локальная панель

Откройте http://192.168.4.1. Панель показывает:
- состояние TWAI;
- принятые кадры, ID, DLC и DATA;
- CAN ID inventory, число кадров и интервалы;
- RX/TX/BUS errors, missed, overrun;
- Wi-Fi и Cloudflare WSS.

## Сборка

Требуется PlatformIO и Espressif32 8.0.0 (Arduino-ESP32 3.3.12).

Команды из корня репозитория:
- pio run -d firmware
- pio run -d firmware -t upload
- pio device monitor -d firmware -b 115200

GitHub Actions собирает прошивку автоматически и публикует firmware.bin как workflow artifact.

## Ограничения

- Сигналы автомобиля не расшифровываются автоматически. Пока анализируются сырые кадры, ID, байты, интервалы и частоты.
- CAN FD не поддерживается.
- Если число кадров остаётся нулевым, проверять нужно физическую CAN-линию, скорость, питание/землю трансивера и подключение к нужной шине.
- Тестовый Cloudflare Worker сейчас без авторизации. Не распространяйте адрес сервера публично.
