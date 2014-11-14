#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include <string>
#define STRCPY(d,s) do { strncpy(d, s, sizeof(d)-1); d[sizeof(d)-1]=0; } while (0)

extern "C" uint64_t IntHash(const char* key, int len, int left, int right);
extern "C" uint64_t StringHash(const char *key, int len ,int left, int right);
extern "C" uint64_t CommentHash(const char *key, int len, int left, int right);
extern "C" uint64_t ActHash(const char* key, int len, int left, int right);
extern "C" uint64_t VideoHash(const char* key, int len, int left, int right);
extern "C" uint64_t LiveHash(const char* key, int len, int left, int right);

std::map<std::string, uint64_t (*)(const char*, int, int, int)> hf_map = \
			{ {std::string("IntHash"), IntHash}, \
			  {std::string("StringHash"), StringHash}, \
			  {std::string("CommentHash"), CommentHash}, \
			  {std::string("ActHash"), ActHash}, \
			  {std::string("VideoHash"), VideoHash}, \
			  {std::string("LiveHash"), LiveHash} };

char key[129];
char func[32];
char default_func[] = "StringHash";
int dbs = 1;
bool dbs_flag = false;
int tables = 1;
bool func_flag = false;


			  
void show_usage(int argc, char **argv) {
	printf("%s [-d db] -t table -k key [-f function]\n", argv[0]);
	printf("    -d db\n");
	printf("        db numbers. default: 1\n");
	printf("    -t table\n");
	printf("        table numbers.\n");
	printf("    -k key\n");
	printf("        key to check, key length should not bigger then 128.\n");
	printf("    -f function\n");
	printf("        hash function to use.\n");
	printf("        available: IntHash, StringHash, CommentHash, ActHash, VideoHash, LiveHash.\n");
	printf("        default: StringHash\n");
}

int parse_argv(int argc, char **argv) {
	char op;
	while ((op = getopt(argc, argv, "k:f:d:t:")) != -1) {
		switch (op) {
		case 'd':
			dbs = atoi(optarg);
			dbs_flag = true;
			break;
		case 't':
			tables = atoi(optarg);
			break;
		case 'k':
			STRCPY(key, optarg);
			break;
		case 'f':
			STRCPY(func, optarg);
			func_flag = true;
			break;
		default:
			fprintf(stderr, "invalid option: %c\n", op);
			show_usage(argc, argv);
			return -1;
		}
	}
	if (func_flag == false)
		STRCPY(func, default_func);
	if (dbs_flag == false)
		dbs = 1;
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		show_usage(argc, argv);
		return -1;
	}
	memset(key, 0, sizeof(key));
	memset(func, 0, sizeof(func));
	if (parse_argv(argc, argv) == -1)
		return -1;
	uint64_t (*func_ptr)(const char*, int, int, int);

	//printf("func: %s\n", func);
	//printf("key: %s\n", key);
	//printf("dbs: %d\n", dbs);
	//printf("tables: %d\n", tables);
	if ((func_ptr = hf_map[std::string(func)]) == NULL) {
		fprintf(stderr, "unknown hash function %s\n", func);
		show_usage(argc, argv);	
		return -1;
	}
	uint64_t hashed = func_ptr(key, strlen(key), 1, -1);
	//printf("hashed: %d\n", hashed);
	printf("key:%s, db: %d, table: %d\n", key, hashed % dbs + 1, hashed / dbs % tables + 1);

	return 0;
}
