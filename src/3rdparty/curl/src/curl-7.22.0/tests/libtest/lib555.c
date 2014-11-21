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

/* This test case is supposed to be identical to 547 except that this uses the
 * multi interface and 547 is easy interface.
 *
 * argv1 = URL
 * argv2 = proxy
 * argv3 = proxyuser:password
 */

#include "test.h"
#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define MULTI_PERFORM_HANG_TIMEOUT 60 * 1000

#define UPLOADTHIS "this is the blurb we want to upload\n"

static size_t readcallback(void  *ptr,
                           size_t size,
                           size_t nmemb,
                           void *clientp)
{
  int *counter = (int *)clientp;

  if(*counter) {
    /* only do this once and then require a clearing of this */
    fprintf(stderr, "READ ALREADY DONE!\n");
    return 0;
  }
  (*counter)++; /* bump */

  if(size * nmemb > strlen(UPLOADTHIS)) {
    fprintf(stderr, "READ!\n");
    strcpy(ptr, UPLOADTHIS);
    return strlen(UPLOADTHIS);
  }
  fprintf(stderr, "READ NOT FINE!\n");
  return 0;
}
static curlioerr ioctlcallback(CURL *handle,
                               int cmd,
                               void *clientp)
{
  int *counter = (int *)clientp;
  (void)handle; /* unused */
  if(cmd == CURLIOCMD_RESTARTREAD) {
    fprintf(stderr, "REWIND!\n");
    *counter = 0; /* clear counter to make the read callback restart */
  }
  return CURLIOE_OK;
}


int test(char *URL)
{
  int res;
  CURL *curl;
  int counter=0;
  CURLM *m = NULL;
  int running=1;
  struct timeval mp_start;
  char mp_timedout = FALSE;

  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    fprintf(stderr, "curl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  if ((curl = curl_easy_init()) == NULL) {
    fprintf(stderr, "curl_easy_init() failed\n");
    curl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  test_setopt(curl, CURLOPT_URL, URL);
  test_setopt(curl, CURLOPT_VERBOSE, 1L);
  test_setopt(curl, CURLOPT_HEADER, 1L);

  /* read the POST data from a callback */
  test_setopt(curl, CURLOPT_IOCTLFUNCTION, ioctlcallback);
  test_setopt(curl, CURLOPT_IOCTLDATA, &counter);
  test_setopt(curl, CURLOPT_READFUNCTION, readcallback);
  test_setopt(curl, CURLOPT_READDATA, &counter);
  /* We CANNOT do the POST fine without setting the size (or choose chunked)! */
  test_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(UPLOADTHIS));

  test_setopt(curl, CURLOPT_POST, 1L);
#ifdef CURL_DOES_CONVERSIONS
  /* Convert the POST data to ASCII. */
  test_setopt(curl, CURLOPT_TRANSFERTEXT, 1L);
#endif
  test_setopt(curl, CURLOPT_PROXY, libtest_arg2);
  test_setopt(curl, CURLOPT_PROXYUSERPWD, libtest_arg3);
  test_setopt(curl, CURLOPT_PROXYAUTH,
                   (long) (CURLAUTH_NTLM | CURLAUTH_DIGEST | CURLAUTH_BASIC) );

  if ((m = curl_multi_init()) == NULL) {
    fprintf(stderr, "curl_multi_init() failed\n");
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  if ((res = (int)curl_multi_add_handle(m, curl)) != CURLM_OK) {
    fprintf(stderr, "curl_multi_add_handle() failed, "
            "with code %d\n", res);
    curl_multi_cleanup(m);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  mp_timedout = FALSE;
  mp_start = tutil_tvnow();

  while (running) {
    static struct timeval timeout = /* 100 ms */ { 0, 100000L };
    fd_set fdread, fdwrite, fdexcep;
    int maxfd = -1;

    res = (int)curl_multi_perform(m, &running);
    if (tutil_tvdiff(tutil_tvnow(), mp_start) >
        MULTI_PERFORM_HANG_TIMEOUT) {
      mp_timedout = TRUE;
      break;
    }
#ifdef TPF
    sleep(1); /* avoid ctl-10 dump */
#endif
    if (running <= 0) {
      fprintf(stderr, "nothing left running.\n");
      break;
    }

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);
    curl_multi_fdset(m, &fdread, &fdwrite, &fdexcep, &maxfd);

    /* In a real-world program you OF COURSE check the return code of the
       function calls.  On success, the value of maxfd is guaranteed to be
       greater or equal than -1.  We call select(maxfd + 1, ...), specially in
       case of (maxfd == -1), we call select(0, ...), which is basically equal
       to sleep. */

    if (select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout) == -1) {
      res = ~CURLM_OK;
      break;
    }
  }

  if (mp_timedout) {
    fprintf(stderr, "mp_timedout\nABORTING TEST, since it seems "
            "that it would have run forever.\n");
    res = TEST_ERR_RUNS_FOREVER;
  }

test_cleanup:

  if(m) {
    curl_multi_remove_handle(m, curl);
    curl_multi_cleanup(m);
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return res;
}

