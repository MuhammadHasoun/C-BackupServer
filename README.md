
# Daemon Server met MySQL en E-mailmeldingen

## Overzicht

Dit project is een Linux daemon-server geschreven in C++ die verbinding maakt met een MySQL-database en e-mailmeldingen verstuurt. Het logt activiteiten en behandelt systeemsignalen voor een nette afsluiting.

## Functionaliteiten

- Draait als een daemon-proces
- Verbindt met een MySQL-database om gegevens op te halen
- Verstuurt e-mails via libcurl SMTP (voorbeeld met Gmail)
- Logt gebeurtenissen naar een logbestand (`/tmp/daemon_server_log.txt`)
- Verwerkt UNIX-signalen voor nette afsluiting

## Vereisten

- Linux omgeving
- MySQL client bibliotheken en server
- libcurl development bibliotheken
- Bouwtools (g++)

## Installatie

1. Installeer de benodigde pakketten (voorbeeld voor Debian/Ubuntu):

```bash
sudo apt-get update
sudo apt-get install libmysqlclient-dev libcurl4-openssl-dev g++
```

2. Compileer het project:

```bash
g++ -o daemon_server daemon_server.cpp -lmysqlclient -lcurl
```

## Configuratie

- Pas de MySQL verbindingsgegevens aan in de broncode:

```cpp
char* server = "localhost";
char* user = "root";
char* password = "voorbeeld"; // Jouw MySQL wachtwoord
char* database = "users";
int mysql_port = 3306;
```

- Pas de SMTP e-mailgegevens aan in de functie `sendEmail`:

```cpp
curl_easy_setopt(curl, CURLOPT_USERNAME, "jouw_email@gmail.com");
curl_easy_setopt(curl, CURLOPT_PASSWORD, "jouw_wachtwoord");
curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "<jouw_email@gmail.com>");
```

## Uitvoering

Start de gecompileerde daemon server:

```bash
./daemon_server
```

Bekijk het logbestand voor activiteit: `/tmp/daemon_server_log.txt`

## Signalen

- Gebruik `Ctrl+C` of stuur SIGINT om de server netjes af te sluiten.

## Opmerkingen

- Gebruik in productie altijd veilige methodes om wachtwoorden op te slaan.
- Denk aan het gebruik van omgevingsvariabelen of configuratiebestanden.
- Gmail SMTP vereist app-wachtwoorden of OAuth2 authenticatie.

## Licentie

Dit project wordt geleverd zonder enige garantie.
