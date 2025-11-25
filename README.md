# Descrizione
 Spada laser interattiva basata su Arduino NANO, che combina controllo preciso dei LEDs WS2812B via assembly, rilevamento di movimento e colpi tramite l'accelerometro MPU6050 e riproduzione di effetti audio sincronizzati.</br>
 Sistema embedded con gestione di stati, colori e suoni.





## Funzionalità
 - Accensione/Spegnimento graduale con feedback sonoro
 - Cambio colore tramite pressione prolungata
 - Suoni di swing e hit attivati da movimento e urti
 - Gestione/Pulizia automatica della memoria RAM
 - Sistema di debug via Serial Monitor





## Tecnologie
 - C++/Arduino per la logica principale
 - Assembly AVR per il controllo preciso dei LEDs
 - I2C per comunicazione con MPU6050
 - SPI per lettura SD card
 - EEPROM per salvataggio configurazioni





## Hardware
 - Arduino NANO v3 ATmega328P CH340 USB-C
 - WS2812B: striscia a led indirizzabile da 144LEDs/m, 256 LEDs in totale
 - MPU6050: giroscopio/accelerometro
 - PAM8403: mini-amplificatore 5V
 - piccolo speaker mono magnetico 8Ohm-1W
 - modulo microSD + scheda microSD
 - pulsante
 - switch ON/OFF
 - batteria 9V-650mAh ricaricabile con USB-C





## Diagramma dei collegamenti
 <img width="1060" height="820" alt="Image" src="https://github.com/user-attachments/assets/e785ca32-0832-4506-8490-6ad3c166e74c" />





## Librerie
 *sketch_aug25a.ino*
 ```
 #include <EEPROM.h>    // Per salvare/caricare in memoria le impostazioni del colore
 #include <SD.h>        // Per leggere i file audio dalla scheda microSD
 #include <SPI.h>       // Per le comunicazioni SPI con la scheda microSD
 #include <Wire.h>      // Per le comunicazioni I2C con l'MPU6050 giroscopio/accelerometro
 #include <TMRpcm.h>    // Per la riproduzione audio tramite file WAV
 ```

 *WS2812B.S*
 ```
 #include "avr/io.h"    ; Definizioni dei registri I/O AVR per il controllo diretto dei LEDs WS2812B
 ```





## Esempio di debug output via Serial Monitor
```
19:49:47.626 -> Free RAM: 2290 bytes
19:49:50.234 -> Lightsaber Starting...
19:49:50.234 -> Free RAM: 2290
19:49:50.266 -> LEDs OK
19:49:50.266 -> Loaded color - R:3 G:60 B:0
19:49:50.299 -> SD Card: OK
19:49:50.299 -> MPU6050: OK
19:49:50.363 -> System Ready
19:49:51.233 -> Free RAM: 2290 bytes
19:49:52.427 -> Button RELEASED - Short press
19:49:52.427 -> Playing sound: ign.wav
19:49:52.459 -> Activating lightsaber
19:49:54.005 -> Free RAM: 673 bytes
19:49:55.548 -> Playing sound: cls.wav
19:49:56.031 -> Free RAM: 673 bytes
19:49:58.027 -> Free RAM: 704 bytes
19:50:01.824 -> Color change mode - HOLD to cycle colors, RELEASE to select
19:50:01.888 -> Color Preview - R:3 G:60 B:0
19:50:01.984 -> Color Preview - R:6 G:57 B:0
19:50:02.113 -> Color Preview - R:9 G:54 B:0
19:50:02.113 -> Free RAM: 704 bytes
19:50:02.209 -> Color Preview - R:12 G:51 B:0
19:50:02.337 -> Color Preview - R:15 G:48 B:0
19:50:02.433 -> Color Preview - R:18 G:45 B:0
19:50:02.561 -> Color Preview - R:21 G:42 B:0
19:50:02.658 -> Color Preview - R:24 G:39 B:0
19:50:02.787 -> Color Preview - R:27 G:36 B:0
19:50:02.884 -> Color Preview - R:30 G:33 B:0
19:50:03.013 -> Color Preview - R:33 G:30 B:0
19:50:03.110 -> Color Preview - R:36 G:27 B:0
19:50:03.239 -> Color Preview - R:39 G:24 B:0
19:50:03.335 -> Color Preview - R:42 G:21 B:0
19:50:03.463 -> Color Preview - R:45 G:18 B:0
19:50:03.558 -> Color Preview - R:48 G:15 B:0
19:50:03.687 -> Color Preview - R:51 G:12 B:0
19:50:03.784 -> Color Preview - R:54 G:9 B:0
19:50:03.913 -> Color Preview - R:57 G:6 B:0
19:50:04.010 -> Color Preview - R:60 G:3 B:0
19:50:04.137 -> Color Preview - R:63 G:0 B:0
19:50:04.202 -> Color SELECTED
19:50:04.234 -> Color saved to EEPROM
19:50:04.234 -> Saved color - R:63 G:0 B:0
19:50:04.330 -> Free RAM: 704 bytes
19:50:07.972 -> Playing sound: sw2.wav
19:50:08.357 -> Free RAM: 673 bytes
19:50:10.325 -> Playing sound: sw1.wav
19:50:12.386 -> Free RAM: 673 bytes
19:50:14.384 -> Free RAM: 704 bytes
19:50:16.155 -> Color change mode - HOLD to cycle colors, RELEASE to select
19:50:16.251 -> Color Preview - R:61 G:0 B:2
19:50:16.316 -> Color Preview - R:58 G:0 B:5
19:50:16.445 -> Color Preview - R:55 G:0 B:8
19:50:16.445 -> Free RAM: 704 bytes
19:50:16.543 -> Color Preview - R:52 G:0 B:11
19:50:16.672 -> Color Preview - R:49 G:0 B:14
19:50:16.769 -> Color Preview - R:46 G:0 B:17
19:50:16.898 -> Color Preview - R:43 G:0 B:20
19:50:16.994 -> Color Preview - R:40 G:0 B:23
19:50:17.123 -> Color Preview - R:37 G:0 B:26
19:50:17.220 -> Color Preview - R:34 G:0 B:29
19:50:17.348 -> Color Preview - R:31 G:0 B:32
19:50:17.444 -> Color Preview - R:28 G:0 B:35
19:50:17.572 -> Color Preview - R:25 G:0 B:38
19:50:17.669 -> Color Preview - R:22 G:0 B:41
19:50:17.797 -> Color Preview - R:19 G:0 B:44
19:50:17.893 -> Color Preview - R:16 G:0 B:47
19:50:18.022 -> Color Preview - R:13 G:0 B:50
19:50:18.119 -> Color Preview - R:10 G:0 B:53
19:50:18.247 -> Color Preview - R:7 G:0 B:56
19:50:18.343 -> Color Preview - R:4 G:0 B:59
19:50:18.474 -> Color Preview - R:1 G:0 B:62
19:50:18.570 -> Color Preview - R:0 G:1 B:62
19:50:18.667 -> Free RAM: 704 bytes
19:50:18.699 -> Color Preview - R:0 G:4 B:59
19:50:18.795 -> Color Preview - R:0 G:7 B:56
19:50:18.923 -> Color Preview - R:0 G:10 B:53
19:50:19.052 -> Color Preview - R:0 G:13 B:50
19:50:19.149 -> Color Preview - R:0 G:16 B:47
19:50:19.278 -> Color Preview - R:0 G:19 B:44
19:50:19.374 -> Color Preview - R:0 G:22 B:41
19:50:19.503 -> Color Preview - R:0 G:25 B:38
19:50:19.600 -> Color Preview - R:0 G:28 B:35
19:50:19.728 -> Color Preview - R:0 G:31 B:32
19:50:19.825 -> Color Preview - R:0 G:34 B:29
19:50:19.954 -> Color Preview - R:0 G:37 B:26
19:50:20.051 -> Color Preview - R:0 G:40 B:23
19:50:20.180 -> Color Preview - R:0 G:43 B:20
19:50:20.277 -> Color Preview - R:0 G:46 B:17
19:50:20.406 -> Color Preview - R:0 G:49 B:14
19:50:20.502 -> Color Preview - R:0 G:52 B:11
19:50:20.631 -> Color Preview - R:0 G:55 B:8
19:50:20.728 -> Color Preview - R:0 G:58 B:5
19:50:20.824 -> Color Preview - R:0 G:61 B:2
19:50:20.857 -> Free RAM: 704 bytes
19:50:20.953 -> Color Preview - R:0 G:63 B:0
19:50:20.953 -> Color SELECTED
19:50:20.985 -> Color saved to EEPROM
19:50:21.017 -> Saved color - R:0 G:63 B:0
19:50:22.820 -> Playing sound: hit.wav
19:50:22.916 -> Free RAM: 673 bytes
19:50:24.945 -> Free RAM: 704 bytes
19:50:24.977 -> Playing sound: sw1.wav
19:50:26.941 -> Free RAM: 673 bytes
19:50:28.970 -> Free RAM: 704 bytes
19:50:28.970 -> Button RELEASED - Short press
19:50:29.002 -> Playing sound: pou.wav
19:50:29.066 -> Deactivating lightsaber
19:50:31.478 -> Free RAM: 704 bytes
```
