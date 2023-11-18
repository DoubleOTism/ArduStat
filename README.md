# ArduStat

## Project Description
This project involves creating a custom thermostat using Arduino. It allows for dynamic temperature control based on time and day, with user-adjustable settings.

## Components Used
- Arduino UNO R3 (or compatible clone)
- BMP280 temperature and pressure sensor
- 1-Channel Relay Module (5V)
- I2C OLED display (1.3" - 128x64, white)
- 4x4 Membrane Keypad
- RTC (Real Time Clock) Module DS1302

## Wiring Guide
- BMP280 -> Arduino (I2C communication)
- OLED Display -> Arduino (I2C communication)
- Relay Module -> Digital Pin (e.g., D2)
- Keypad -> Digital Pins (e.g., D6-D13)
- RTC Module -> Digital Pins (e.g., D3-D5)

## Features
- Adjustable temperature settings for different times and days
- Real-time temperature and pressure display
- Manual relay override (On, Off, Auto)
- EEPROM memory storage for settings

# ArduStat

## Popis Projektu
Tento projekt zahrnuje vytvoření vlastního termostatu s použitím Arduina. Umožňuje dynamickou regulaci teploty na základě času a dne, s možností uživatelského nastavení.

## Použité Komponenty
- Arduino UNO R3 (nebo kompatibilní klon)
- Teplotní a tlakový senzor BMP280
- Jednokanálový relé modul (5V)
- I2C OLED displej (1.3" - 128x64, bílý)
- Membránová klávesnice 4x4
- RTC (Real Time Clock) Modul DS1302

## Návod na Zapojení
- BMP280 -> Arduino (komunikace I2C)
- OLED Displej -> Arduino (komunikace I2C)
- Relé Modul -> Digitální Pin (např. D2)
- Klávesnice -> Digitální Piny (např. D6-D13)
- RTC Modul -> Digitální Piny (např. D3-D5)

## Funkce
- Nastavitelné teplotní režimy pro různé časy a dny
- Zobrazení aktuální teploty a tlaku v reálném čase
- Manuální přepínání relé (Zapnuto, Vypnuto, Auto)
- Ukládání nastavení v paměti EEPROM

