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

typedef int bool;
#define true 1
#define false 0

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

char* rlm_metalive_LogName(uint32_t log_mach) {
  switch (log_mach) {
  case 0:
    return "Main";
  case 1:
    return "Aux 1";
  case 2:
    return "Aux 2";
  }

  return "";
}

char* rlm_metalive_LogMode(uint32_t log_mode) {
  switch (log_mode) {
  case 1:
    return "LiveAssist";
  case 2:
    return "Automatic";
  case 3:
    return "Manual";
  }

  return "";
}

void rlm_metalive_JsonStringAttribute(char *json, char *name, const char *value) {
  if (strlen(value) == 0) {
    return;
  }

  char escaped_value[512];
  strcpy(escaped_value, value);
  rlm_metalive_Replace(escaped_value, '"', "\\\"");

  char json_attribute[512];
  snprintf(json_attribute,512,"\"%s\":\"%s\"", name, escaped_value);

  if (strlen(json) > 0) {
    strcat(json, ",");
  }
  strcat(json, json_attribute);
}

void rlm_metalive_JsonIntAttribute(char *json, char *name, const int32_t value) {
  char json_attribute[512];
  snprintf(json_attribute,512,"\"%s\":%d", name, value);

  if (strlen(json) > 0) {
    strcat(json, ",");
  }
  strcat(json, json_attribute);
}

void rlm_metalive_JsonBooleanAttribute(char *json, char *name, const bool value) {
  char json_attribute[512];
  snprintf(json_attribute,512,"\"%s\":%s", name, value ? "true" : "false");

  if (strlen(json) > 0) {
    strcat(json, ",");
  }
  strcat(json, json_attribute);
}

void rlm_metalive_JsonAttributes(char *json, const struct rlm_log *log, const struct rlm_pad *pad) {
  if (pad->rlm_cartnum == 0) {
    return;
  }

  rlm_metalive_JsonIntAttribute(json, "number", pad->rlm_cartnum);
  rlm_metalive_JsonIntAttribute(json, "length", pad->rlm_len);

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

  rlm_metalive_JsonStringAttribute(json, "conductor", pad->rlm_conductor);
  rlm_metalive_JsonStringAttribute(json, "song_id", pad->rlm_song_id);
  rlm_metalive_JsonStringAttribute(json, "outcue", pad->rlm_outcue);
  rlm_metalive_JsonStringAttribute(json, "description", pad->rlm_description);

  rlm_metalive_JsonBooleanAttribute(json, "onair", log->log_onair == 1);
  rlm_metalive_JsonStringAttribute(json, "log_machine", rlm_metalive_LogName(log->log_mach));
  rlm_metalive_JsonStringAttribute(json, "log_name", log->log_name);
  rlm_metalive_JsonStringAttribute(json, "log_mode", rlm_metalive_LogMode(log->log_mode));
}

void rlm_metalive_RLMPadDataSent(void *ptr,const struct rlm_svc *svc,
				const struct rlm_log *log,
				const struct rlm_pad *now,
				const struct rlm_pad *next)
{
  RLMLog(ptr,LOG_INFO,"rlm_metalive: send new event");

  if (rlm_metalive_key != NULL) {
    char url[1024];
    snprintf(url,512,"http://metalive.tryphon.eu/api/streams/%s/events.json", rlm_metalive_key);

    /* char debug[1024]; */
    /* snprintf(debug, 512, "onair: %c",log->log_onair); */
    /* RLMLog(ptr,LOG_DEBUG,debug); */

    char now_json[2048];

    strcpy(now_json, "");
    rlm_metalive_JsonAttributes(now_json, log, now);

    char next_json[2048];

    strcpy(next_json, "");
    rlm_metalive_JsonAttributes(next_json, log, next);

    if (strlen(now_json) > 0) {
      strcat(now_json, ",");
    }

    char json[4096];
    snprintf(json,4096, "{\"description\": {%s\"next\": {%s}}}", now_json, next_json);

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
