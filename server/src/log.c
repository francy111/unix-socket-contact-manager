/*
 * Copyright (c) 2024 Biribo' Francesco, Giannuzzi Riccardo, Timour Ilyas
 *
 * Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "./../include/log.h"
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

void formatMessage(logMessage *msg, char *_client, char *_operation, short int _success, char *_additionalMsg) {

    // Svuotiamo il contenuto del paccheto per rimuovere eventuali dati non voluti
    memset(msg->dateTime, '\0', DATE_TIME_MAX_LENGTH);
    memset(msg->client, '\0', CLIENT_MAX_LENGTH);
    memset(msg->operation, '\0', OPERATION_MESSAGE_MAX_LENGTH);
    memset(msg->additionalMsg, '\0', ADDITIONAL_MESSAGE_MAX_LENGTH);

    time_t timeInSeconds;
    struct tm structuredTime;
    char timeStringBuffer[DATE_TIME_MAX_LENGTH];

    time(&timeInSeconds); // Restituisce i secondi passati da gennaio 1 1970 12AM
    localtime_r(&timeInSeconds, &structuredTime); // Lo formatta in una struttura contenente giorno, mese, anno e ora
    asctime_r(&structuredTime, timeStringBuffer); // Restituisce la struttura della data in una stringa direttamente formattata per la lettura umana
    timeStringBuffer[strlen(timeStringBuffer) - 1] = '\0'; // L'ultimo carattere e' \n e non lo vogliamo
    strncpy(msg->dateTime, timeStringBuffer, strlen(timeStringBuffer));
    strncpy(msg->client, _client, strlen(_client));
    strncpy(msg->operation, _operation, strlen(_operation));
    msg->success = _success;
    if(_additionalMsg != NULL) strncpy(msg->additionalMsg, _additionalMsg, strlen(_additionalMsg));
}

void logF(logMessage msg) {
    /*
     * Apertura del file
     *  - Sola scrittura
     *  - Modalita' append, le scritture avvengono sempre in fondo al file
     *  - Il file viene creato se non esiste
     */
    int logFd = open("files/log.txt", O_WRONLY | O_APPEND | O_CREAT, 0664);

    // Formattazione della stringa da scrivere
    char str[DATE_TIME_MAX_LENGTH + CLIENT_MAX_LENGTH + OPERATION_MESSAGE_MAX_LENGTH + ADDITIONAL_MESSAGE_MAX_LENGTH + 15];
    memset(str, '\0', DATE_TIME_MAX_LENGTH + CLIENT_MAX_LENGTH + OPERATION_MESSAGE_MAX_LENGTH + ADDITIONAL_MESSAGE_MAX_LENGTH + 15);
    sprintf(str, "%s - Author: %s - %s", msg.dateTime, msg.client, msg.operation);

    if(msg.success != IGNORED) { 
        sprintf(str + strlen(str), msg.success == SUCCESS ? " - SUCCEEDED" : " - FAILED");
    }
    if(msg.additionalMsg != NULL) {
        sprintf(str + strlen(str), " - %s", msg.additionalMsg);
    }
    sprintf(str + strlen(str), "\n");

    // Scrittura e chiusura file
    write(logFd, str, strlen(str));
    close(logFd);
}