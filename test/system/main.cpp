#include <gtest/gtest.h>
#include "Process.h"

bool runPms(const std::string& args) {
    return system(("src/PlantowerPmsLinux " + args).c_str());
}

TEST(Pms, Main) {
    EXPECT_EQ(0, runPms("-h"));
}


TEST(System, RunOnce) {
    EXPECT_EQ(0, runPms("-1"));
}

TEST(System, RunContinuous) {
    const char *args[] = { "src/PlantowerPmsLinux", nullptr};
    auto pms = Process::runCommand(-1, args);
    sleep(5);
    pms.signal(SIGTERM);
    EXPECT_EQ(0, pms.wait());
}

TEST(System, RunTimed) {
    const char *args[] = { "src/PlantowerPmsLinux", "-t" , "3", nullptr};
    auto pms = Process::runCommand(-1, args);
    sleep(10);
    pms.signal(SIGTERM);
    EXPECT_EQ(0, pms.wait());
}
