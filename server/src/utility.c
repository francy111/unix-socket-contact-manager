/*
 * Copyright (c) 2024 Biribo' Francesco, Giannuzzi Riccardo, Timour Ilyas
 *
 * Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "./../include/utility.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

int getContact(Contact *cntc, int index) {

    // Apro il file rubrica.txt in lettura
    int fd = open("files/rubrica.txt", O_RDONLY, 0666);
    int contactIndex = 0;

    // Un contatto al massimo occupa tanto spazio quanto 3 campi, 2 virgole per la separazione e uno terminatore
    int lineMaxLength = 3 * CONTACT_STRINGS_LENGTH + 2 + 1;
    char chr, line[lineMaxLength];
    memset(line, '\0', lineMaxLength);
    int done = 0;

    // Leggo una riga alla volta. Ogni riga contiene un contatto, quindi leggere 1 riga corrisponde a un contatto
    while(!done && readLine(fd, line) > 0) {
        if(contactIndex == index) // Controlla se siamo arrivato all'n-esimo contatto della rubrica
            done = 1;
        else 
            memset(line, '\0', lineMaxLength);
        contactIndex++;
    }

    // Se è stato trovato il contatto (puo' non succedere se l'indice richiesto è oltre i limiti della rubrica)
    if(done) {

        /* 
         * Siccome i contatti sono salvati come [nome,cognome,numeroTelefono]
         * è sufficiente separare il contenuto della linea fermandoci quando troviamo
         * una virgola
         * 
         * I dati che leggiamo li trascriviamo nei campi del cntc
         */
        char *token = strtok(line, ",");
        strncpy(cntc->name, token, strlen(token));

        token = strtok(NULL, ",");
        strncpy(cntc->surname, token, strlen(token));

        token = strtok(NULL, ",");
        strncpy(cntc->phoneNumber, token, strlen(token));
    } 

    close(fd);
    return done;
}

int addContact(Contact cntc) {

    // Apro la rubrica in lettura, append ed eventualmente la creo se non esiste
    umask(0);
    int fd = open("files/rubrica.txt", O_RDWR | O_APPEND | O_CREAT, 0666);
    int added = 0, present = 0;

    // Vogliamo procedere solo se siamo riusciti ad aprire il file
    if(fd > -1) {

        // Un contatto al massimo occupa tanto spazio quanto 3 campi, 2 virgole per la separazione e uno terminatore
        int writeLen = 3 * CONTACT_STRINGS_LENGTH + 2 + 1 + 1;
        char toWrite[writeLen], line[writeLen];
        memset(toWrite, '\0', writeLen);
        sprintf(toWrite, "%s,%s,%s", cntc.name, cntc.surname, cntc.phoneNumber);

        //Leggiamo un contatto alla volta per controllare se esisteva gia' o meno
        while(readLine(fd, line) > 0) {
            if(strncmp(toWrite, line, strlen(toWrite)) == 0) {
                present = 1;
            }
            memset(line, '\0', writeLen);
        }

        // Lo aggiungiamo solo se non era gia' presente, è la terna ad essere univoca
        if(present == 0) {
            toWrite[strlen(toWrite)] = '\n';
            added = write(fd, toWrite, strlen(toWrite));

            if(added > 0) 
                added = 1;
            else 
                added = 0;
        } else {
            /*
            * Serve per indicare che non è stato aggiunto perchè è un duplicato
            * 0 viene utilizzato come esito quando si verifica un errore nell'apertura del file
            */ 
            added = 2;
        }
        close(fd);
    }
    return added;
}
int removeContact(Contact cntc) {

    // Apro la rubrica in lettura
    umask(0);
    int fd = open("files/rubrica.txt", O_RDONLY, 0666);
    int removed = 0, present = 0, aborted = 0;

    // Procediamo solo se è stato aperto il file
    if(fd > -1) {

        // Un contatto al massimo occupa tanto spazio quanto 3 campi, 2 virgole per la separazione e uno terminatore
        int writeLen = 3 * CONTACT_STRINGS_LENGTH + 2 + 1 + 1;
        char line[writeLen];
        memset(line, '\0', writeLen);
        
        /* 
         * Per rimuovere il contatto utilizziamo un file temporaneo
         * che utilizzeremo per copiare ogni contatto della rubrica 
         * originaria, tranne quello da eliminare, successivamente
         * ridenominiamo il file temporaneo, sostituendo la vecchia rubrica
         */
        int tmpFile = open("files/tmp", O_WRONLY | O_CREAT, 0666);

        char toDel[writeLen];
        memset(toDel, '\0', writeLen);
        sprintf(toDel, "%s,%s,%s", cntc.name, cntc.surname, cntc.phoneNumber);

        // Controllo ogni linea (contatto)
        while(readLine(fd, line) > 0) {

            if(strncmp(toDel, line, strlen(toDel))) { // Se non è quello che voglio eliminare lo copio
                line[strlen(line)] = '\n';
                if(write(tmpFile, line, strlen(line)) < 0) {
                    close(tmpFile);
                    unlink("files/tmp");
                    removed = 0;
                    aborted = 0;
                    break;
                }
            } else { // Altrimenti non lo trascrivo
                present = 1;
                removed = 1;
            }
            memset(line, '\0', writeLen);
        }

        /*
         * Serve per indicare che non è stato rimosso perchè non c'era
         * 0 viene utilizzato come esito quando si verifica un errore nell'apertura del file
         */ 
        if(!present) removed = 2;

        // Chiudo entrambi i descriptor 
        if(!aborted) close(tmpFile);
        close(fd);

        // Rimpiazzo della rubrica
        if(!aborted) rename("files/tmp", "files/rubrica.txt");
    }
    return removed;
}

int modifyContact(Contact old, Contact new) {

    // Apro la rubrica in sola lettura
    umask(0);
    int fd = open("files/rubrica.txt", O_RDONLY, 0666);
    int modified = 0, present = 0, aborted = 0;

    // Procediamo solo se riusciamo ad aprire il file
    if(fd > -1) {
        
        // Un contatto al massimo occupa tanto spazio quanto 3 campi, 2 virgole per la separazione e uno terminatore
        int writeLen = 3 * CONTACT_STRINGS_LENGTH + 2 + 1 + 1;
        char line[writeLen];
        memset(line, '\0', writeLen);

        /* 
         * Per modificare il contatto utilizziamo un file temporaneo
         * che utilizzeremo per copiare ogni contatto della rubrica 
         * originaria, tranne quello da modificare, in questo caso
         * non trascriveremo il contatto originale ma quello nuovo,
         * successivamente ridenominiamo il file temporaneo, 
         * ottenendo una sostituzione a tutti gli effetti
         */
        int tmpFile = open("files/tmp", O_WRONLY | O_CREAT, 0666);

        char toModify[writeLen];
        memset(toModify, '\0', writeLen);
        sprintf(toModify, "%s,%s,%s", old.name, old.surname, old.phoneNumber);

        // Controlliamo una linea alla
        while(readLine(fd, line) > 0) {

            if(strncmp(toModify, line, strlen(toModify))) { // Se non è quello che devo modificare lo copio
                line[strlen(line)] = '\n';
                if(write(tmpFile, line, strlen(line)) < 0) {
                    modified = 0;
                    aborted = 1;
                    close(tmpFile);
                    break;
                }
            } else { // Il contatto da modificare viene rimpiazzato, trascrivendo quello nuovo al suo posto
                modified = 1;
                present = 1;
                memset(line, '\0', writeLen);
                sprintf(line, "%s,%s,%s", new.name, new.surname, new.phoneNumber);
                line[strlen(line)] = '\n';
                if(write(tmpFile, line, strlen(line)) < 0) {
                    modified = 0;
                    aborted = 1;
                    close(tmpFile);
                    break;
                }
            }
            memset(line, '\0', writeLen);
        }

        /*
         * Serve per indicare che non è stato modificato perchè non c'era
         * 0 viene utilizzato come esito quando si verifica un errore nell'apertura del file
         */ 
        if(!present) modified = 2;
        if(!aborted) close(tmpFile);
        close(fd);
        if(!aborted) rename("files/tmp", "files/rubrica.txt");
    }
    return modified;
}

int matchesParameters(Contact asked, Contact found) {
    int matching = 1;

    /* Controlliamo i parametri, se un parametro è vuoto, non lo consideriamo per il confronto
     * N.B. found è sempre un contatto dalla rubrica, con tutti e 3 i campi presenti,
     * asked proviene della richiesta del client, il quale puo' richiedere semplicemente
     * delle corrispondenze che alcuni contatti hanno con alcuni dei parametri
     * 
     * Semplicemente, se un parametro è vuoto significa che il client non era interessato
     * a trovare un contatto con un valore particolare per quel campo, quindi lo ignoriamo
     * nel confronto
     */

    // Controlliamo se il nome combacia, se serve controllarlo
    if(asked.name[0] != '\0') {
        if((int)strcmp(asked.name, found.name)) {
            matching = 0;
        }
    }

    // Controlliamo se il cognome combacia, se serve controllarlo
    if(asked.surname[0] != '\0') {
        if((int)strcmp(asked.surname, found.surname)) {
            matching = 0;
        }
    }

    // Controlliamo se il nomero di telefono combacia, se serve controllarlo
    if(asked.phoneNumber[0] != '\0') {
        if((int)strcmp(asked.phoneNumber, found.phoneNumber)) {
            matching = 0;
        }
    }

    return matching;
}

void createEmptyContact(Contact *cntc) {

    // Inizializza ogni campo di contact a stringhe piene di zeri
    memset(cntc->name, '\0', CONTACT_STRINGS_LENGTH + 1);
    memset(cntc->surname, '\0', CONTACT_STRINGS_LENGTH + 1);
    memset(cntc->phoneNumber, '\0', CONTACT_STRINGS_LENGTH + 1);
}

int isContactEmpty(Contact cntc) {

    // Se il primo carattere è \0 la stringa è lunga 0 (corrisponde a "")
    return cntc.name[0] == '\0' && cntc.surname[0] == '\0' && cntc.phoneNumber[0] == '\0';
}

int checkCredentials(char *username, char *password) {

    // Le credenziali vengono salvate come coppia (user,password), che è univoca, quindi cerchiamo nel file una linea che corrisponde alla coppia
    int credentialSize = AUTH_STRINGS_LENGTH + (HASH_LENGTH) + 2;
    char asked[credentialSize], found[credentialSize];

    char hashString[HASH_LENGTH + 1];
    
    memset(asked, '\0', credentialSize);
    memset(found, '\0', credentialSize);
    memset(hashString, '\0', HASH_LENGTH + 1);

    /*
     * Il client invia la password in chiaro, ma nel file è salvato il suo hash
     * quindi prima calcoliamo il suo hash e cerchiamo poi la coppia
     * (user, pswHash)
     */
    hashFunction(password, hashString);

    sprintf(asked, "%s,%s", username, hashString);

    // Apriamo il file credenziali in lettura
    int fd = open("files/credenziali.txt", O_RDONLY, 0666);
    int done = 0, index = 0;

    char chr;

    // Leggiamo una linea alla volta fino a che è stata trovata la coppia o fino a che non finiscono line su file
    while(!done && readLine(fd, found) > 0) {
        if(strcmp(asked, found) == 0) 
            done = 1;
    }
    return done;
}

int readLine(int fd, char buf[]) {

    /*
     * Supponiamo che la open sia stata fatta, e che fd sia il file descriptor
     * di tale file. Questo perchè per poter leggere una linea alla volta dobbiamo
     * tenere presente il puntatore interno al file (contenuto in fd), quindi non
     * avrebbe senso fare open() e close() all'interno della funzione.
     * Di quello si dovra' occupare il chiamante prima e dopo
     */
    char chr;
    int index = 0, done = 0;

    // Controlliamo comunque che il fd passato sia corretto
    if(fd > -1) {

        // Leggiamo un carattere alla volta
        while(!done && read(fd, &chr, 1) > 0) {

            // Quando troviamo uno \n siamo arrivati a fine linea
            if(chr == '\n') {
                buf[index] = '\0'; // Lo sostituiamo con uno \0 in quanto il newline fa parte solo della formattazione per la scrittura su file, non si usa all'esterno di esso
                done = 1;
            } else {    // Finchè non trovo newline sto costruendo via via la linea
                buf[index] = chr;
                index++;
            }
        }
    } else {
        // Siccome incrementiamo index con ogni carattere scritto, indica quanti caratteri scritti, per indicare un errore di apertura del file impostiamolo a -1
        index = -1;
    }
    
    return index;
}

void hashFunction(char *toHash, char *hash) {

    unsigned long long hashTmp = 5381;

    for(int i = 0; i < strlen(toHash); i++) {
        hashTmp = (hashTmp << 5) + hashTmp + toHash[i];
    }

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charsetSize = (int)strlen(charset);
    for(int i = 0; i < HASH_LENGTH; i++) {
        hash[i] = charset[hashTmp % charsetSize];
        hashTmp /= charsetSize;
    }
    hash[HASH_LENGTH] = '\0';
}

int addUser(char *username, char *password) {

    // Un utente al massimo occupa tanto spazio quanto username e password, 1 virgola e 1 carattere finale (terminatore nella stringa, sostituito in newline nel file)
    int lineLen = AUTH_STRINGS_LENGTH + 1 + HASH_LENGTH + 1 + 1;
    char line[lineLen];
    memset(line, '\0', lineLen);

    // Apriamo il file credenziali sia in scrittura che lettura, in append, ed eventualmente lo creiamo
    umask(0);
    int fd = open("files/credenziali.txt", O_RDWR | O_APPEND | O_CREAT, 0666);
    int added = 0;
    
    // Procediamo solo se il file è stato aperto
    if(fd > -1) {

        // Leggiamo una riga alla volta tutto il file credenziali
        int alreadyPresent = 0;
        while(!alreadyPresent && readLine(fd, line) > 0) {

            // Controlliamo carattere per carattere che le stringhe siano identiche e della stessa lunghezza
            int i = 0;
            alreadyPresent = 1;
            while((line[i] != ',' || username[i] != '\0') && i < AUTH_STRINGS_LENGTH) {
                if(username[i] != line[i]){
                    alreadyPresent = 0;
                    i = AUTH_STRINGS_LENGTH; // Termina il ciclo se non sono uguali
                }
                i++;
            }
        }

        // Se non è presente lo aggiungo
        if(!alreadyPresent) {
            char hashedPassword[HASH_LENGTH + 1];
            memset(line, '\0', lineLen);

            // Calcolo l'hash della password
            hashFunction(password, hashedPassword);

            // Formattiamo la stringa con la coppia (username, passwordHash)
            memset(line, '\0', lineLen);
            sprintf(line, "%s,%s", username, hashedPassword);
            line[strlen(line)] = '\n';

            // La scriviamo
            if(write(fd, line, strlen(line)) > 0)
                added = 1;
        }
        close(fd);
    } else {
        added = 2;
    }
    return added;
}

int removeUser(char *username) {

    // Un utente al massimo occupa tanto spazio quanto username e password, 1 virgola e 1 carattere finale (terminatore nella stringa, sostituito in newline nel file)
    int lineLen = AUTH_STRINGS_LENGTH + 1 + HASH_LENGTH + 1 + 1;
    char line[lineLen];
    memset(line, '\0', lineLen);

    // Apriamo il file credenziali in lettura
    umask(0);
    int fd = open("files/credenziali.txt", O_RDONLY);
    int removed = 0, aborted = 0;

    // Procediamo solo se il file è stato aperto
    if(fd > - 1) {

        /* 
         * Per rimuovere l'utente utilizziamo un file temporaneo
         * che utilizzeremo per copiare ogni utente, tranne quello
         * da rimuovere, in questo caso.
         * Siccome un username appartiene al massimo ad un utente
         * è sufficiente fare la ricerca per username 
         * Infine sostituiamo il file originale con quello temporaneo
         */
        int tmp = open("files/tmpUsers", O_CREAT | O_WRONLY, 0666);

        // Leggiamo una riga alla volta, nella forma [user,password]
        while(readLine(fd, line) > 0) { // Ogni riga la copiamo tranne quella da togliere

            /*
             * Facendo il controllo carattere per carattere
             * abbiamo la garanzia che l'unico modo per avere un riscontro positivo
             * è che sia lo stesso utente. Sapendo che l'utente è univoco, non abbiamo
             * dubbi che è quello da cancellare
             */

            int i = 0, sameUsername = 1;
            // Controlliamo carattere per carattere che le stringhe siano identiche e della stessa lunghezza
            while((line[i] != ',' || username[i] != '\0') && i < AUTH_STRINGS_LENGTH) {
                if(username[i] != line[i]){
                    sameUsername = 0;
                    i = AUTH_STRINGS_LENGTH; // Termina il ciclo se non sono uguali
                }
                i++;
            }
            if(!sameUsername) { // Trascriviamo tutti gli utenti
                line[strlen(line)] = '\n';
                if(write(tmp, line, strlen(line)) < 0) {
                    aborted = 1;
                    removed = 0;
                    close(tmp);
                    break;
                }
            } else { // Tranne quello da cancellare
                removed = 1;
            }
            memset(line, '\0', lineLen);
        }

        // Chiudiamo i file descriptor di entrambi i file e sostituiamo il file
        if(!aborted) close(tmp);
        close(fd);
        if(!aborted) rename("files/tmpUsers", "files/credenziali.txt");
    } else {
        removed = 2;
    } 
    return removed;
}

void cleanInputBuffer(void) {
    int c;
    while((c = getchar()) != '\n' && c != EOF) {
        // Scarta tutti i caratteri fino a newline o EOF
    }
}

long getOptionalInput(char* string, size_t size) {
    if(fgets(string, size, stdin) != NULL){
        size_t len = strlen(string);

        // Controlla se la stringa è lunga quanto il massimo e se l'ultimo carattere è una newline (Invio dell'utente)
        if (len == size - 1 && string[len-1] != '\n')
            cleanInputBuffer(); // Se non c'è il newline allora la stringa inserita dall'utente era troppo lunga e il buffer è sicuramente sporco
        
        if (len > 0 && string[len-1] == '\n') {
            string[len-1] = '\0'; // Rimuove il newline
        } 
        return len;
    } else {
        return -1;
    }
}

char getSingleChar(void){
    // acquisisco dall'utente una stringa di massimo 2 caratteri + '\0'
    char input[3] = {'x','x','\0'};
    getOptionalInput(input, 3);
    // controlliamo se il secondo carattere è vuoto (quindi se l'utente ha digitato un solo carattere)
    if(input[1] == '\0')
        return input[0]; // se è un solo carattere lo restituisce
    else
        return EOF; // uso EOF come carattere per il caso di errore
}

void disableEcho(void) {
    struct termios term;

    // Ottiene le impostazioni correnti del terminale
    tcgetattr(STDIN_FILENO, &term);

    // Disabilita l'ECHO, non mostrando l'input
    term.c_lflag &= ~ECHO;

    // Applica le nuove impostazioni
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void enableEcho(void) {
    struct termios term;

    // Ottiene le impostazioni correnti del terminale
    tcgetattr(STDIN_FILENO, &term);

    // Abilita l'ECHO, per mostrare l'input
    term.c_lflag |= ECHO;

    // Applica le nuove impostazioni
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int isUsernameValidAndNotEmpty(char *username){
    size_t len = strlen(username); 
    int result = 0;
    // Non deve essere vuoto ma nemmeno più lungo di 20 caratteri
    if(0 < len && len < AUTH_STRINGS_LENGTH + 1) { // Compreso tra 1 e 20
        result = 1;
        for(int i = 0; i < len; i++){
            if(!(isDigit(username[i]) || isLetter(username[i]))) // Se non è una lettera/cifra non è valido
                result = 0;
        }
    }
    return result;
}

int isPasswordValid(char *password){
    size_t len = strlen(password); 
    int result = 0;
    if(len < AUTH_STRINGS_LENGTH + 1) { // Minore o uguale a 20
        result = 1;
        for(int i = 0; i < len; i++){
            if(!(isDigit(password[i]) || isLetter(password[i]))) // Se non è una lettera/cifra non è valido
                result = 0;
        }
    }
    return result;
}

int isDigit(char c) {
    return c >= '0' && c <= '9';
}

int isLetter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}