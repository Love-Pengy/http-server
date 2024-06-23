#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
char *itoa(int input) {
  // get a given value by dividing by a multiple of 10
  //  check if it's the end by first getting the lenght by seeing if it
  //  divides by a multiple without being a fraction
  int currProduct = 1;
  int size = 0;
  char *output = malloc(sizeof(char) * 22);
  bool entered = false;
  output[0] = '\0';
  if (input == 0) {
    output[0] = '0';
    output[1] = '\0';
  }
  while (!((input / currProduct) < 1)) {
    if (currProduct != 1) {
      entered = true;
    }
    size++;
    currProduct *= 10;
  }
  currProduct = 1;
  for (int i = 0; i < size; i++) {
    // 0th index is highest one
    // add 48 because of ascii table
    output[i] = (char)((
        ((int)(input / (int)powl((long)10, (long)((size - 1) - i))) % 10) +
        48));
    currProduct *= 10;
  }
  output[size] = '\0';
  if (!entered) {
    output[0] = (char)(input + 48);
    output[1] = '\0';
  }
  return (output);
}

int main(int argc, char **argv) {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Server Started");

  // Uncomment this block to pass the first stage

  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;
  // creates ipv4 endpoint that handles two way connections
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }
  //
  // Since the tester restarts your program quite often, setting
  // SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }

  // struct that specifies the type, port number, and address of the socket
  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };

  // binds the addr struct created above to the socket we created
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  // marks server_fd as a passive socket (one that is used to accept incoming
  // requests)
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);
  while (1) {
    int fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    char *tokBuff = malloc(sizeof(char) * 10000);
    tokBuff[0] = '\0';
    int bytesRead = read(fd, tokBuff, 10000);
    printf("%s\n", tokBuff);
    int bytesSent = 0;
    char *filename;
    if (argc > 1) {
      for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--directory")) {
          char *inputCopy = malloc(sizeof(char) * 1001);
          memcpy(inputCopy, tokBuff, strlen(tokBuff));
          strtok(inputCopy, " ");
          char *tmp = strtok(NULL, " ");
          strtok(tmp, "/");
          filename = strtok(NULL, "/");
          break;
        }
      }
      if (strstr(tokBuff, "POST")) {
        char *buffCopy = malloc(sizeof(char) * 1001);
        buffCopy[0] = '\0';
        memcpy(buffCopy, tokBuff, strlen(tokBuff));
        char *requestVal;
        char *tmp;
        tmp = strtok(buffCopy, "\r\n\r\n");
        if (tmp != NULL) {
          requestVal = tmp;
        }
        while ((tmp = strtok(NULL, "\r\n\r\n"))) {
          requestVal = tmp;
        }
        char *fullPath = malloc(sizeof(char) * 1024);
        fullPath[0] = '\0';
        snprintf(fullPath, strlen(filename) + strlen(argv[2]) + 1, "%s%s",
                 argv[2], filename);
        FILE *fp = fopen(fullPath, "w");
        fprintf(fp, requestVal);
        fclose(fp);
        char *responseString = malloc(sizeof(char) * 1024);
        responseString[0] = '\0';
        strncpy(responseString, "HTTP/1.1 201 Created\r\n\r\n", 80);
        bytesSent = send(fd, responseString, strlen(responseString), 0);
        continue;
      }
      char *filepath = malloc(sizeof(char) * 1024);
      filepath[0] = '\0';
      snprintf(filepath, strlen(filename) + strlen(argv[2]) + 1, "%s%s",
               argv[2], filename);
      errno = 0;
      FILE *fp = fopen(filepath, "r");
      if (errno) {
        strerror(errno);
        char *responseString = malloc(sizeof(char) * 1024);
        responseString[0] = '\0';
        strncpy(responseString, "HTTP/1.1 404 Not Found\r\n\r\n", 32);
        bytesSent = send(fd, responseString, strlen(responseString), 0);
        continue;
      }
      fseek(fp, 0, SEEK_END);
      int fileSize = ftell(fp);
      rewind(fp);
      char *fileSizeString = itoa(fileSize);
      char *fileContents = malloc(sizeof(char) * (fileSize + 1));
      fread(fileContents, fileSize, 1, fp);
      char *responseString = malloc(sizeof(char) * 1024);
      responseString[0] = '\0';
      snprintf(responseString, strlen(fileSizeString) + 35 + 47 + 5 + 1,
               "%s%s%s",
               "HTTP/1.1 200 OK\r\nContent-Type: "
               "application/octet-stream\r\nContent-Length: ",
               fileSizeString, "\r\n\r\n");
      strncat(responseString, fileContents, strlen(fileContents));
      bytesSent = send(fd, responseString, strlen(responseString), 0);
      fclose(fp);
      continue;
    } else if (strstr(tokBuff, "User-Agent:")) {
      char *curr = strtok(tokBuff, " ");
      while (curr = strtok(NULL, " ")) {
        if (strstr(curr, "User-Agent:")) {
          break;
        }
      }
      char *echoVal = strtok(NULL, " ");
      char *outputString = malloc(sizeof(char) * 1024);
      outputString[0] = '\0';
      char *responseString = malloc(sizeof(char) * 1024);
      responseString[0] = '\0';
      int echoSize = strlen(echoVal) - 4;
      char *echoString = itoa(echoSize);
      snprintf(
          responseString, strlen(echoString) + 31 + 28 + 5 + 1, "%s%s%s",
          "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ",
          echoString, "\r\n\r\n");
      strncat(responseString, echoVal, strlen(echoVal));
      bytesSent = send(fd, responseString, strlen(responseString), 0);
    } else if (strstr(tokBuff, "echo")) {
      strtok(tokBuff, "/");
      strtok(NULL, "/");
      char *tmpTok2 = strtok(NULL, "/");
      char *echoVal = strtok(tmpTok2, " ");
      char *outputString = malloc(sizeof(char) * 1024);
      outputString[0] = '\0';
      char *responseString = malloc(sizeof(char) * 1024);
      responseString[0] = '\0';
      int echoSize = strlen(echoVal);
      char *echoString = itoa(echoSize);
      snprintf(
          responseString, strlen(echoString) + 31 + 28 + 5 + 1, "%s%s%s",
          "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ",
          echoString, "\r\n\r\n");
      strncat(responseString, echoVal, strlen(echoVal));
      bytesSent = send(fd, responseString, strlen(responseString), 0);
    } else {
      char *successResponse = "HTTP/1.1 200 OK\r\n\r\n";
      char *failResponse = "HTTP/1.1 404 Not Found\r\n\r\n";
      strtok(tokBuff, " ");
      char *filespec = strtok(NULL, " ");
      int bytessent = 0;
      if (!strcmp(filespec, "/")) {
        bytessent = send(fd, successResponse, strlen(successResponse), 0);
      } else {
        bytessent = send(fd, failResponse, strlen(failResponse), 0);
      }
      if (bytessent < 1) {
        printf("no bytes sent");
      }
    }
    if (bytesSent == 0) {
      printf("Nothing Sent\n");
    }
    printf("Client connected\n");
  }

  close(server_fd);

  return 0;
}
