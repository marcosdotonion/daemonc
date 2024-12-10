#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <sys/inotify.h>
#include <libnotify/notify.h>

#define EXT_SUCESS 0
#define EXT_ERR_TOO_FEW_ARGS 1
#define EXT_ERR_INIT_INOTIFY 2
#define EXT_ERR_ADD_WATCH 3
#define EXT_ERR_BASE_PATH_NULL 4
#define EXT_ERR_READ_INOTIFY 5
#define EXT_ERR_INIT_LIBNOTIFY 6


int IeventQueue = -1;
int IeventStatus = -1;

char *ProgramTitle = "daemonite";

void signal_handler(int signal) {
    //(void)signal; //SUPRESSED NON-USED signal;
    int closeStatus = -1;
    printf("Signal received, cleaning...\n");

    closeStatus = inotify_rm_watch(IeventQueue, IeventStatus);
    if (closeStatus == -1) {
        fprintf(stderr, "Error removing file from our watch\n");
    }

    close(IeventQueue);
    exit(EXIT_SUCCESS);

}

int main(int argc, char** argv) {

    bool libnotifyInitStatus = false;


    char *basePath = NULL; //POINTERS GO HERE!!!    
    char *token = NULL;
    char *notificationMessage = NULL;


    char buffer[4096];
    int readLength;

    const struct inotify_event* watchEvent;

    const uint32_t watchMask = IN_CREATE | IN_DELETE | IN_ACCESS | IN_CLOSE_WRITE | IN_MODIFY | IN_MOVE_SELF;

    if (argc < 2) {
        fprintf(stderr, "USAGE: daemonite PATH\n");
        exit(EXT_ERR_TOO_FEW_ARGS);
    }  

    basePath = (char *)malloc(sizeof(char)*(strlen(argv[1]) + 1));
    strcpy(basePath, argv[1]);

    token = strtok(basePath, "/");
    while (token != NULL) {
        basePath = token;
        token = strtok(NULL, "/");
    }

    if (basePath == NULL) {
        fprintf(stderr, "Error. again. Path this time LOL\n");
        exit(EXT_ERR_BASE_PATH_NULL);
    }

    libnotifyInitStatus = notify_init(ProgramTitle);
    if (!libnotifyInitStatus) {
        fprintf(stderr, "Error initializing libnotify!\n");
        exit(EXT_ERR_INIT_LIBNOTIFY);
    }

    libnotifyInitStatus = notify_init(ProgramTitle);

    IeventQueue = inotify_init();
    if (IeventQueue == -1) {
        fprintf(stderr, "Error initializing inotify instance\n");
        exit(EXT_ERR_INIT_INOTIFY);
    }

    IeventStatus = inotify_add_watch(IeventQueue, argv[1], watchMask);
    if (IeventStatus == -1) {
        fprintf(stderr, "Error adding file to watch instance!\n");
        exit(EXT_ERR_ADD_WATCH);
    }

    signal(SIGABRT, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (true) {
        printf("Waiting for ievent...\n");

        readLength = read(IeventQueue, buffer, sizeof(buffer));
        if (readLength == -1) {
            fprintf(stderr, "Error reading from inotify instance!");
            exit(EXT_ERR_READ_INOTIFY);
        }

        for (char *bufferPointer = buffer; bufferPointer < buffer + readLength; bufferPointer += sizeof(struct inotify_event) + watchEvent->len) {
            
            notificationMessage = NULL;
            watchEvent = (const struct inotify_event *) bufferPointer;

            if (watchEvent->mask & IN_CREATE) {
                notificationMessage = "File created.\n";
            }

            if (watchEvent->mask & IN_DELETE) {
                 notificationMessage = "File deleted.\n";
            }

            if (watchEvent->mask & IN_ACCESS) {
                 notificationMessage = "File accessed.\n";
            }

            if (watchEvent->mask & IN_CLOSE_WRITE) {
                 notificationMessage = "File written and closed.\n";
            }

            if (watchEvent->mask & IN_MODIFY) {
                 notificationMessage = "File modified.\n";
            }

            if (watchEvent->mask & IN_MOVE_SELF) {
                 notificationMessage = "File moved.\n";
            }

            if (notificationMessage == NULL) {
                continue;
            }

            //printf("%s\n", notificationMessage);
            NotifyNotification *notifyHandle = NULL;
            notifyHandle = notify_notification_new(basePath, notificationMessage, "dialog-information");
            if (notifyHandle == NULL) {
                fprintf(stderr, "notification handle null.\n");
                continue;
            }

            notify_notification_show(notifyHandle, NULL);
        }
    }
}
