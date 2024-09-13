#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BACKUP_DIR "/home/ldeadlysinx/바탕화면/testbackup"  // 백업 디렉토리 이름
#define BUFFER_SIZE 1024

volatile sig_atomic_t stop_backup = 0;  // 신호 처리 플래그

// SIGINT 핸들러 함수: 백업 중지 플래그 설정
void handle_sigint(int signo) {
    stop_backup = 1;
    printf("신호 확인 백업 중지\n");
}

// 파일 백업 함수 (쓰레드에서 실행될 함수)
void *backup_file(void *filename) {
    char *file = (char *)filename;
    char src_path[BUFFER_SIZE], dest_path[BUFFER_SIZE];

    // 소스 파일 전체 경로 지정
    snprintf(src_path, sizeof(src_path), "%s/%s", "/home/ldeadlysinx/바탕화면/test", file);
    
    // 백업 디렉토리로의 전체 경로 지정
    snprintf(dest_path, sizeof(dest_path), "%s/%s", BACKUP_DIR, file);

    FILE *src_file = fopen(src_path, "rb");
    printf("src_path:%s",src_path);
    if (src_file == NULL) {
        perror("파일을 여는데 실패");
        pthread_exit(NULL);
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
        perror("백업 파일 만드는데 실패");
        fclose(src_file);
        pthread_exit(NULL);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);

    printf("백업 성공 파일: %s\n", file);
    pthread_exit(NULL);
}

// 디렉토리에서 파일 목록을 읽고, 파일마다 백업 쓰레드를 생성
void backup_files(const char *directory) {
    DIR *dir;
    struct dirent *entry;
    pthread_t threads[100];
    int thread_count = 0;

    // 백업 디렉토리가 없으면 생성
    if (access(BACKUP_DIR, F_OK) == -1) {
        mkdir(BACKUP_DIR, 0755);
    }

    // 디렉토리 열기
    if ((dir = opendir(directory)) == NULL) {
        perror("디렉토리 여는데 실패");
        exit(EXIT_FAILURE);
    }

     // 디렉토리 내 파일 목록 읽기
    while ((entry = readdir(dir)) != NULL && !stop_backup) {
        if (entry->d_type == DT_REG) {  // 일반 파일인 경우
            
            

                printf("Found file: %s\n", entry->d_name);
                char *file_name = strdup(entry->d_name);  // 파일 이름 복사

                // 파일 백업을 위한 쓰레드 생성
                if (pthread_create(&threads[thread_count], NULL, backup_file, file_name) != 0) {
                    perror("스레드를 만드는데 에러 발생");
                    free(file_name);
                    continue;
                }

                thread_count++;
            
        }
    }
    closedir(dir);

    // 모든 쓰레드 종료 대기
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_to_backup>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // SIGINT 신호 처리기 설정
    signal(SIGINT, handle_sigint);

    // 백업 시작
    printf("디렉토리에 백업 시작중: %s\n", argv[1]);
    while (!stop_backup) {
        backup_files(argv[1]);

        if (stop_backup) break;

        // 10초 후 다시 백업 수행
        printf("다음 백업 시스템 대기중\n");
        sleep(10);
    }

    printf("백업 프로세스 삭제중\n");
    return 0;
}
