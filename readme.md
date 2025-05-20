
# Daemon Server

Deze daemon server luistert naar inkomende verbindingen en verwerkt commando's van clients, zoals het simuleren van toetsenbordinvoer.

## Functies
- Luisteren op een TCP-poort voor inkomende clients
- Ontvangen van commando's in JSON-formaat
- Simuleren van toetsenbordinvoer op basis van ontvangen commando's
- Logging van activiteit

## Installatie

Volg deze stappen om de daemon server op te zetten en te starten:

1. **Systeemvereisten**
   - Linux-systeem (Ubuntu/Debian aanbevolen)
   - Python 3.6 of hoger
   - `python-evdev` (voor toetsenbord simulatie)

2. **Python en vereisten installeren**

```bash
sudo apt update
sudo apt install python3 python3-pip python3-evdev
```

3. **Daemon server bestanden downloaden**

Download de bestanden van de server naar een directory op je machine.

4. **Start de daemon server**

Ga naar de directory met de servercode en voer uit:

```bash
python3 daemon_server.py
```

De server zal nu luisteren op de standaard TCP-poort (bijv. 12345).

5. **Firewall instellingen**

Zorg ervoor dat de TCP-poort open staat in je firewall, bijvoorbeeld:

```bash
sudo ufw allow 12345/tcp
```

## Gebruik

Verbind een client met de daemon server op het juiste IP-adres en poort, en stuur JSON-commando's zoals:

```json
{
  "action": "type",
  "text": "Hallo wereld"
}
```

## Opmerkingen

- Deze daemon moet draaien met voldoende rechten om toetsenbordinvoer te simuleren.
- Pas eventueel de poort en configuraties aan in de servercode.
