#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <random>
#include <thread>
#include <algorithm>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "logger.hpp"

using namespace std;

static const char *dirname = "./dir";
static const char *logname = "./dir/unittest";
static const char *log_link_name = "./dir/unittest.log";

std::string generateRandomString(int len) {
    std::string ret;
    std::random_device rd;
    for (int i = 0; i < len; ++i) {
        ret.push_back(rd() % 26 + 'a');
    }
    return ret;
}

void removeDirent() {
    if (access(dirname, F_OK) == -1) {
        return;
    }
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        std::cerr << "Failed to open directory." << std::endl;
        exit(1);
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) { 
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        string name = string(dirname) + "/" + string(entry->d_name);
        if (remove(name.c_str()) != 0) {
            std::cerr << "Error deleting file" << endl;
            exit(1);
        }
    }
    closedir(dir);
    if (rmdir(dirname) == -1) {
        std::cerr << "Error deleting directory" << std::endl;
        printf("Error deleting directory: %s\n", strerror(errno));
        exit(1);
    }
}

void createLogDirent() {
    removeDirent();
    if (mkdir(dirname, 0777) == -1) {
        std::cerr << "Error create directory" << std::endl;
        exit(1);
    }
}

std::vector<std::string> getAllLogfiles() {
  std::vector<std::string> ret;
  std::string folderPath(dirname);
  DIR* dir = opendir(folderPath.c_str());
  if (dir == NULL) {
    std::cerr << "Failed to open directory." << std::endl;
    exit(1);
  }
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) { 
    if (entry->d_type == DT_REG) {
      ret.push_back(entry->d_name);
    }
  }
  closedir(dir);
  return ret;
}

class LogUnittestHelper {
public:
    /* setup the log-path */
    LogUnittestHelper(const char *name)
        :filename(name),
        contents() {
        ifstream f(filename);
    }
    ~LogUnittestHelper() {
        if (remove(filename.c_str()) != 0) {
            std::cerr << "Error deleting file" << endl;
            exit(1);
        }
        contents.clear();
    }
    LogUnittestHelper(const LogUnittestHelper &) = delete;
    /* 通过当前的软链接文件找到所有的文件 */
    void constructLogContent() {
        std::ifstream f(filename.c_str());
        if (!f.is_open()) {
            std::cerr << "failed to open file: " << filename << std::endl;
            exit(1);
        }
        std::string line;
        int count = 0;
        while (getline(f, line)) {
            if (++count > 3) {
                contents.emplace_back(line);
            }
        }
        f.close();

        /* Find the pattern, and get the message detail. */
        string pattern = " " + string(1, getThreadLocalIdentifier()) + " ";
        for (int i = 0; i < contents.size(); ++i) {
            size_t where = contents[i].find(pattern);
            if (where == string::npos) {
                cout << "error foramt: " << contents[i] << endl;
                exit(1);
            }
            where += pattern.size();
            contents[i] = contents[i].substr(where, contents[i].size() - where);
        }
    }
    /* 检查是否存在该日志内容 */
    bool checkExist(const string &str) {
        for (size_t i = 0; i < contents.size(); ++i) {
            if (contents[i] == str) {
                return true;
            }
        }
        printf("cannot found %s", str.c_str());
        return false;
    }
    /* 检查是否包含某条日志内容 */
    bool checkRoughly(const vector<string> &input_contents) {
        if (contents.size() != input_contents.size()) {
            cout << "size not equal: " << contents.size() << " != " << input_contents.size() << endl;
            return false;
        }
        for (int i = 0; i < contents.size(); ++i) {
            if (contents[i].find(input_contents[i]) == string::npos) {
                cout << "diff: " << contents[i] << " do not find " << input_contents[i] << endl;
                return false;
            }
        }
        return true;
    }
    /* 检查日志内容是否与输入内容完全一致 */
    bool checkExactly(const vector<string> &input_contents) {
        if (contents.size() != input_contents.size()) {
            cout << "size not equal: " << contents.size() << " != " << input_contents.size() << endl;
            return false;
        }
        for (int i = 0; i < contents.size(); ++i) {
            if (contents[i] != input_contents[i]) {
                cout << "diff: " << contents[i] << " != " << input_contents[i] << endl;
                return false;
            }
        }
        return true;
    }
    size_t logSize() {
        return contents.size();
    }
private:
    string filename;
    vector<string> contents;
};

class LogTest : public testing::TestWithParam<int> {
protected:
    virtual void SetUp() override {
        createLogDirent();
    }
    virtual void TearDown() override {
        /* 防止生成重复的文件名称 */
        sleep(1);
    }
    LogUnittestHelper log_helper{log_link_name};
};

TEST_F(LogTest, basicTest) {
    initLoggingSystem(logname);
    setLogLevel(LOG_INFO);
    /* Do not sync wait. */
    setLogSyncLevel(LOG_FATAL);
    logInfo("this is a info log");
    logWarning("this is a warning log");
    logError("this is a error log");
    deinitLoggingSystem();

    log_helper.constructLogContent();
    log_helper.checkExist("this is a info log");
    log_helper.checkExist("this is a warning log");
    log_helper.checkExist("this is a error log");
}

TEST_F(LogTest, highLevelLogPersistent) {
    initLoggingSystem(logname);
    setLogLevel(LOG_INFO);
    setLogSyncLevel(LOG_ERROR);
    logError("This is an error log, expect find it after call.");
    log_helper.constructLogContent();
    ASSERT_EQ(log_helper.logSize(), 1);
    ASSERT_EQ(log_helper.checkExist(string("This is an error log, expect find it after call.")), 1);
    deinitLoggingSystem();
}

/* 单个线程大量写入，rotate怎么处理? */

TEST_F(LogTest, batchWrite) {
    initLoggingSystem(logname);
    setLogLevel(LOG_INFO);
    setLogSyncLevel(LOG_ERROR);
    
    const int log_count = 100000;
    const int log_len = 100;
    vector<string> contents;
    contents.reserve(log_count);
    for (int i = 0; i < log_count; ++i) {
        string log_entry = generateRandomString(log_len);
        logInfo(log_entry.c_str());
        contents.push_back(log_entry);
    }
    deinitLoggingSystem();
    
    log_helper.constructLogContent();
    ASSERT_EQ(log_helper.checkExactly(contents), true);
}

TEST_F(LogTest, concurrentBatchWrite) {
    initLoggingSystem(logname);
    setLogLevel(LOG_INFO);
    setLogSyncLevel(LOG_ERROR);

    const int log_count = 3000;
    const int log_len = 100;
    const int thread_count = 10;
    std::thread threads[thread_count];
    vector<vector<string>> contents(thread_count);
    
    for (int i = 0; i < log_count; ++i) {
        for (int j = 0; j < thread_count; ++j) {
            string log_entry = generateRandomString(log_len);
            contents[j].push_back(log_entry);
        }
    }

    auto concurrent_write = [](const vector<string> &input) {
        for (int i = 0; i < input.size(); ++i) {
            logInfo(input[i].c_str());
            usleep(10);
        }
    };
    for (int i = 0; i < thread_count; ++i) {
        threads[i] = thread(concurrent_write, contents[i]);
    }
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    deinitLoggingSystem();
    log_helper.constructLogContent();
    for (int i = 0; i < contents.size(); ++i) {
        for (int j = 0; j < contents[i].size(); ++j) {
            ASSERT_EQ(true, log_helper.checkExist(contents[i][j]));
        }
    }
}

TEST_F(LogTest, logEverySecondsCheck) {
    initLoggingSystem(logname);
    setLogLevel(LOG_INFO);
    setLogSyncLevel(LOG_ERROR);

    for (int i = 0; i < 10; ++i) {
        logEverySeconds(LOG_WARNING, 5, "logger location1");
        logEverySeconds(LOG_WARNING, 5, "logger location2");
        logEverySeconds(LOG_WARNING, 5, "logger location3");
        logEverySeconds(LOG_WARNING, 5, "logger location4");
        logEverySeconds(LOG_WARNING, 5, "logger location5");
    }

    deinitLoggingSystem();
    log_helper.constructLogContent();
    ASSERT_EQ(log_helper.logSize(), 5);
    ASSERT_EQ(log_helper.checkExist(string("logger location1")), true);
    ASSERT_EQ(log_helper.checkExist(string("logger location2")), true);
    ASSERT_EQ(log_helper.checkExist(string("logger location3")), true);
    ASSERT_EQ(log_helper.checkExist(string("logger location4")), true);
    ASSERT_EQ(log_helper.checkExist(string("logger location5")), true);
}

TEST_F(LogTest, logRotate) {
    const int log_len = 100;
    const int log_count = 10000;
    initLoggingSystem(logname);
    setLogLevel(LOG_INFO);
    setLogSyncLevel(LOG_ERROR);
    /* 1MB大小的文件 */
    setMaxFileSize(1);
    
    vector<string> contents;
    contents.reserve(log_count);
    for (int i = 0; i < log_count; ++i) {
        string log_entry = generateRandomString(log_len);
        logInfo(log_entry.c_str());
        contents.push_back(log_entry);
    }
    deinitLoggingSystem();
    
    auto all_files = getAllLogfiles();
    ASSERT_EQ(all_files.size(), 2);

    /* Check the contents. */    
    LogUnittestHelper helper1((string("./dir/") + all_files[0]).c_str());
    LogUnittestHelper helper2((string("./dir/") + all_files[1]).c_str());
    helper1.constructLogContent();
    helper2.constructLogContent();
    ASSERT_EQ(helper1.logSize() + helper2.logSize(), log_count);

    /* 检查内容 */
    size_t size = max(helper1.logSize(), helper2.logSize());
    vector<string> first_file_contents;
    vector<string> second_file_contents;
    first_file_contents.assign(contents.begin(), contents.begin() + size);
    second_file_contents.assign(contents.begin() + size, contents.end());

    if (helper1.logSize() < helper2.logSize()) {
        helper1.checkExactly(second_file_contents);
        helper2.checkExactly(first_file_contents);
    } else {
        helper1.checkExactly(first_file_contents);
        helper2.checkExactly(second_file_contents);
    }
}
