#  Akıllı Sera Otomasyon Sistemi (Smart Greenhouse Automation)

Bu proje; düşük güç tüketimli, otonom ve MSP430 mikrodenetleyici tabanlı bir akıllı sera otomasyon sistemidir. Sistem, bitki gelişimi için kritik olan çevre parametrelerini anlık olarak izler, bu verileri anlamlandırarak otonom kontrol mekanizmalarını devreye sokar ve bitki sağlığını maksimize eder.

## Öne Çıkan Özellikler

- **Otonom Sulama ve Parazit Koruması:** Toprak nemi kritik eşiğin altına düştüğünde su pompası devreye girer. Röle ve motorun yaratabileceği elektriksel parazitleri önlemek amacıyla özel "vur-kaç" (darbeli) sulama algoritması geliştirilmiştir.
- **Ters Mantık Kalibrasyonu:** Toprak nem sensörünün ters orantılı analog yapısı (kuru = yüksek voltaj, ıslak = düşük voltaj) yazılımsal olarak kalibre edilmiştir.
- **PWM Tabanlı Adaptif Aydınlatma:** Işık sensöründen (LDR) alınan verilere göre, bitkinin ihtiyaç duyduğu yapay ışık desteği PWM ile kademeli olarak ayarlanır.
- **7 Günlük Veri Günlüğü ve Büyüme Skoru:** Sistem, 7 günlük nem, sıcaklık ve ışık verilerinin ortalamasını hafızasında tutarak özel bir "Günlük Büyüme Skoru" hesaplar.
- **Bluetooth (UART) Raporlama:** Elde edilen anlık durum raporları ve geçmiş veriler, Bluetooth modülü üzerinden kablosuz olarak aktarılır.

##  Kullanılan Donanımlar

* **Mikrodenetleyici:** TI MSP430G2553
* **Sensörler:** Toprak Nem Sensörü, LDR (Işık Sensörü), Analog Sıcaklık Sensörü
* **Aktüatörler:** Su Pompası, Röle Modülü, LED Aydınlatma
* **Haberleşme:** HC-05 / HC-06 Bluetooth Modülü

##  Yazılım Mimarisi ve Kod Yapısı

Projenin tüm donanım kontrolü, sensör kalibrasyonu ve karar mekanizmaları tek bir C dosyası (`main.c`) üzerinde toplanmıştır. Modüler yapı sayesinde ana döngü (main loop) karmaşadan uzak tutulmuştur.

## Kurulum ve Kullanım

1. Bu depoyu bilgisayarınıza klonlayın veya zip olarak indirin.
2. Code Composer Studio (CCS) veya IAR üzerinden yeni bir MSP430G2553 projesi oluşturun.
3. Repodaki `main.c` dosyasının içeriğini kendi projenize dahil edin.
4. Derleme (Build) işleminin ardından kodu mikrodenetleyiciye yükleyin (Flash).
5. Bluetooth üzerinden bir seri terminal (9600 baud rate) ile bağlanarak sistem durumunu canlı olarak izleyebilirsiniz.
