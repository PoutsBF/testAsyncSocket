# PempEranda
PempEranda, c'est Température Véranda, contracté en TempÉranda... Mais avec une faute de frappe, c'est devenu PempÉranda... Le 'P' est loin du 'T' ? Allez comprendre... 
2020-09-05 - Création du projet.

## objectif
L'objectif de ce projet est de servir de 'bac à sable' pour prendre en main divers sujets listés ci-dessous. Le support de cet objectif est un objet mesurant la température et l'humidité de la véranda pour maitriser la situation et définir des améliorations, modification, ou RAZ de la susdite véranda...
Les sujets sont :
- prise en main de FreeRTOS (tâches, sémaphores, sections critiques, supervision de la charge des 2 coeurs)
- intégration d'un RTC
- Mesure à partir d'un capteur classique (au départ DTH11, puis BME680)
- Intégration de la connexion wifi
- serveur web et web socket pour la visu temps réel
- page web avec affichage d'une jauge en TR
- enregistrement local de l'historique (csv)
- page web avec affichage du graphique de l'historique
- enregistrement distant de l'historique (mySQL / Raspberrypi)

## sources
A compléter
FreeRTOS ; Adafruit pour le BME680 ; ESPHome pour la gestion web asynchrone ; ...

### FreeRTOS
<http://www.FreeRTOS.org/>

### Gestion web server asynchrone :
<https://github.com/me-no-dev>

### BME680 :
<https://github.com/adafruit/Adafruit_BME680>

### RTC
<https://github.com/adafruit/RTClib>

# Résultats
A compléter...

![entete](documentation/screen1.PNG?raw=true "entête")
![jauges](documentation/screen2.PNG?raw=true "jauges")
![graphiques](documentation/screen3.PNG?raw=true "graphiques")
![base](documentation/screen4.PNG?raw=true "base")
