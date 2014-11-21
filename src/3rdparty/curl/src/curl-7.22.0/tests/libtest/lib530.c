/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "test.h"

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define MAIN_LOOP_HANG_TIMEOUT     90 * 1000
#define MULTI_PERFORM_HANG_TIMEOUT 60 * 1000

#define NUM_HANDLES 4

int test(char *URL)
{
  int res = 0;
  CURL *curl[NUM_HANDLES];
  int running;
  char done=FALSE;
  CURLM *m;
  int i, j;
  struct timeval ml_start;
  struct timeval mp_start;
  char ml_timedout = FALSE;
  char mp_timedout = FALSE;
  char target_url[256];

  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    fprintf(stderr, "curl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  if ((m = curl_multi_init()) == NULL) {
    fprintf(stderr, "curl_multi_init() failed\n");
    curl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* get NUM_HANDLES easy handles */
  for(i=0; i < NUM_HANDLES; i++) {
    curl[i] = curl_easy_init();
    if(!curl[i]) {
      fprintf(stderr, "curl_easy_init() failed "
              "on handle #%d\n", i);
      for (j=i-1; j >= 0; j--) {
        curl_multi_remove_handle(m, curl[j]);
        curl_easy_cleanup(curl[j]);
      }
      curl_multi_cleanup(m);
      curl_global_cleanup();
      return TEST_ERR_MAJOR_BAD + i;
    }
    sprintf(target_url, "%s%04i", URL, i + 1);
    target_url[sizeof(target_url) - 1] = '\0';

    res = curl_easy_setopt(curl[i], CURLOPT_URL, target_url);
    if(res) {
      fprintf(stderr, "curl_easy_setopt() failed "
              "on handle #%d\n", i);
      for (j=i; j >= 0; j--) {
        curl_multi_remove_handle(m, curl[j]);
        curl_easy_cleanup(curl[j]);
      }
      curl_multi_cleanup(m);
      curl_global_cleanup();
      return TEST_ERR_MAJOR_BAD + i;
    }

    /* go verbose */
    res = curl_easy_setopt(curl[i], CURLOPT_VERBOSE, 1L);
    if(res) {
      fprintf(stderr, "curl_easy_setopt() failed "
              "on handle #%d\n", i);
      for (j=i; j >= 0; j--) {
        curl_multi_remove_handle(m, curl[j]);
        curl_easy_cleanup(curl[j]);
      }
      curl_multi_cleanup(m);
      curl_global_cleanup();
      return TEST_ERR_MAJOR_BAD + i;
    }

    /* include headers */
    res = curl_easy_setopt(curl[i], CURLOPT_HEADER, 1L);
    if(res) {
      fprintf(stderr, "curl_easy_setopt() failed "
              "on handle #%d\n", i);
      for (j=i; j >= 0; j--) {
        curl_multi_remove_handle(m, curl[j]);
        curl_easy_cleanup(curl[j]);
      }
      curl_multi_cleanup(m);
      curl_global_cleanup();
      return TEST_ERR_MAJOR_BAD + i;
    }

    /* add handle to multi */
    if ((res = (int)curl_multi_add_handle(m, curl[i])) != CURLM_OK) {
      fprintf(stderr, "curl_multi_add_handle() failed, "
              "on handle #%d with code %d\n", i, res);
      curl_easy_cleanup(curl[i]);
      for (j=i-1; j >= 0; j--) {
        curl_multi_remove_handle(m, curl[j]);
        curl_easy_cleanup(curl[j]);
      }
      curl_multi_cleanup(m);
      curl_global_cleanup();
      return TEST_ERR_MAJOR_BAD + i;
    }
  }

  curl_multi_setopt(m, CURLMOPT_PIPELINING, 1L);

  ml_timedout = FALSE;
  ml_start = tutil_tvnow();

  fprintf(stderr, "Start at URL 0\n");

  while (!done) {
    fd_set rd, wr, exc;
    int max_fd;
    struct timeval interval;

    interval.tv_sec = 1;
    interval.tv_usec = 0;

    if (tutil_tvdiff(tutil_tvnow(), ml_start) >
        MAIN_LOOP_HANG_TIMEOUT) {
      ml_timedout = TRUE;
      break;
    }
    mp_timedout = FALSE;
    mp_start = tutil_tvnow();

    res = (int)curl_multi_perform(m, &running);
    if (tutil_tvdiff(tutil_tvnow(), mp_start) >
        MULTI_PERFORM_HANG_TIMEOUT) {
      mp_timedout = TRUE;
      break;
    }
    if (running <= 0) {
      done = TRUE; /* bail out */
      break;
    }

    if (res != CURLM_OK) {
      fprintf(stderr, "not okay???\n");
      break;
    }

    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&exc);
    max_fd = 0;

    if (curl_multi_fdset(m, &rd, &wr, &exc, &max_fd) != CURLM_OK) {
      fprintf(stderr, "unexpected failured of fdset.\n");
      res = 189;
      break;
    }

    if (select_test(max_fd+1, &rd, &wr, &exc, &interval) == -1) {
      fprintf(stderr, "bad select??\n");
      res = 195;
      break;
    }

    res = CURLM_CALL_MULTI_PERFORM;
  }

  if (ml_timedout || mp_timedout) {
    if (ml_timedout) fprintf(stderr, "ml_timedout\n");
    if (mp_timedout) fprintf(stderr, "mp_timedout\n");
    fprintf(stderr, "ABORTING TEST, since it seems "
            "that it would have run forever.\n");
    res = TEST_ERR_RUNS_FOREVER;
  }

/* test_cleanup: */

  /* cleanup NUM_HANDLES easy handles */
  for(i=0; i < NUM_HANDLES; i++) {
    curl_multi_remove_handle(m, curl[i]);
    curl_easy_cleanup(curl[i]);
  }

  curl_multi_cleanup(m);
  curl_global_cleanup();

  return res;
}
