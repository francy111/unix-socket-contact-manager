/*
 * Copyright (c) 2024 Biribo' Francesco, Giannuzzi Riccardo, Timour Ilyas
 *
 * Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define SUCCESS 1
#define FAILURE 0
#define IGNORED -1

#define DATE_TIME_MAX_LENGTH 30
#define CLIENT_MAX_LENGTH 40
#define OPERATION_MESSAGE_MAX_LENGTH 350
#define ADDITIONAL_MESSAGE_MAX_LENGTH 200

/**
 * Struttura utilizzata per il logging dei messaggi
 * riunisce informazioni utili in un'unica struct
 * 
 * Campi:
 *  dateTime - Data e ora a cui avviene l'operazione
 *  client - Chi ha richiesto l'operazione
 *  operation - Descrive brevemente l'operazione eseguita
 *  success - Indica successo (SUCCESS) o fallimento (FAILURE) dell'operazione, puo' essere ignorato l'esito (IGNORED)
 *  additionalMsg - Puo' essere specificata per indicare informazioni aggiuntive
 */
typedef struct {
    char dateTime[DATE_TIME_MAX_LENGTH];
    char client[CLIENT_MAX_LENGTH];
    char operation[OPERATION_MESSAGE_MAX_LENGTH];
    short int success; 
    char additionalMsg[ADDITIONAL_MESSAGE_MAX_LENGTH];
} logMessage;

/**
 * Formatta il messaggio msg con le informazioni fornite
 * La data viene impostata automaticamente durante l'esecuzione
 * 
 * La funzione permette di preparare in anticipo i dati, anche
 * separatamente, e di creare il messaggio per il logging
 * insieme in un'unica linea
 */
void formatMessage(logMessage *msg, char *_client, char *_operation, short int _success, char *_additionalMsg);

/**
 * Scrive le informazioni contenute nel messaggio msg
 * nel file "log.txt"
 */
void logF(logMessage msg);