/* rlm_metalive.c
 *
 * To compile this module, just do:
 *
 *   gcc -shared -o rlm_metalive.rlm rlm_metalive.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "rlm.h"

char *rlm_metalive_key;

void rlm_metalive_RLMStart(void *ptr,const char *arg)
{
  RLMLog(ptr,LOG_INFO,"rlm_metalive: start");
  rlm_metalive_key = strdup(arg);
}


void rlm_metalive_RLMFree(void *ptr)
{
  RLMLog(ptr,LOG_INFO,"rlm_metalive: free");
}


void rlm_metalive_Replace(char *str, char replace, char *with) {
  if (strlen(str) == 0) {
    return;
  }

  int with_len = strlen(with);
  int index;

  for (index=0; index < (strlen(str)-1); index++) {
    if (str[index]==replace) {
      strcpy(str + index + with_len, str + index + 1);
      strncpy(str + index, with, with_len);
      index += with_len;
    }
  }
}

void rlm_metalive_JsonStringAttribute(char *json, char *name, const char *value) {
  if (strlen(value) == 0) {
    return;
  }

  char escaped_value[512];
  strcpy(escaped_value, value);
  rlm_metalive_Replace(escaped_value, '"', "\\\"");

  char json_attribute[512];
  snprintf(json_attribute,512,"\"%s\": \"%s\"", name, escaped_value);

  if (strlen(json) > 0) {
    strcat(json, ",");
  }
  strcat(json, json_attribute);
}

void rlm_metalive_JsonAttributes(char *json, const struct rlm_pad *pad) {
  rlm_metalive_JsonStringAttribute(json, "album", pad->rlm_album);
  rlm_metalive_JsonStringAttribute(json, "artist", pad->rlm_artist);
  rlm_metalive_JsonStringAttribute(json, "composer", pad->rlm_comp);
  rlm_metalive_JsonStringAttribute(json, "group", pad->rlm_group);
  rlm_metalive_JsonStringAttribute(json, "label", pad->rlm_label);
  rlm_metalive_JsonStringAttribute(json, "publisher", pad->rlm_pub);
  rlm_metalive_JsonStringAttribute(json, "title", pad->rlm_title);
  rlm_metalive_JsonStringAttribute(json, "year", pad->rlm_year);

  rlm_metalive_JsonStringAttribute(json, "agency", pad->rlm_agency);
  rlm_metalive_JsonStringAttribute(json, "client", pad->rlm_client);
  rlm_metalive_JsonStringAttribute(json, "userdef", pad->rlm_userdef);

  rlm_metalive_JsonStringAttribute(json, "isci", pad->rlm_isci);
  rlm_metalive_JsonStringAttribute(json, "isrc", pad->rlm_isrc);
}

void rlm_metalive_RLMPadDataSent(void *ptr,const struct rlm_svc *svc,
				const struct rlm_log *log,
				const struct rlm_pad *now,
				const struct rlm_pad *next)
{
  RLMLog(ptr,LOG_INFO,"rlm_metalive: send new event");

  if (rlm_metalive_key != NULL) {
    char url[1024];
    snprintf(url,512,"http://metalive.tryphon.dev/api/streams/%s/events.json", rlm_metalive_key);

    char now_json[2048];

    strcpy(now_json, "");
    rlm_metalive_JsonAttributes(now_json, now);

    char json[4096];
    snprintf(json,4096, "{\"description\": {%s}}", now_json);

    RLMLog(ptr,LOG_DEBUG,json);

    int pipe_fd[2];
    pipe(pipe_fd);

    pid_t childpid;

    if((childpid = fork()) == -1) {
      RLMLog(ptr,LOG_WARNING,"rlm_metalive: can't fork");
      return;
    }

    if (childpid == 0) {
      close(pipe_fd[1]);
      close(0);
      dup(pipe_fd[0]);
      execlp("curl","curl","-X","POST","-d", "@-", "-o","/dev/null","-s", url,(char *)NULL);
      RLMLog(ptr,LOG_WARNING,"rlm_metalive: unable to execute curl(1)");
      exit(0);
    } else {
      close(pipe_fd[0]);
      write(pipe_fd[1],json,strlen(json));
      close(pipe_fd[1]);
    }
  } else {
    RLMLog(ptr,LOG_WARNING,"rlm_metalive: no key defined");
  }
}
