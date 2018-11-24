#include <gtest/gtest.h>
#include "Properties.h"
#include "PmsMock.h"
#include "../util/SocketPair.h"
#include <iostream>
#include "Process.h"

using namespace std;

TEST(Pms, DISABLED_Main) {
    EXPECT_EQ(0, system("src/PlantowerPmsLinux -h"));
}


TEST(Process, Main) {
    const char * successArgs[] = {"true", nullptr};
    EXPECT_EQ(0, Process::runCommand(-1, successArgs).wait());

    const char * falseArgs[] = {"bash", "-c", "exit 3", nullptr};
    EXPECT_EQ(3, Process::runCommand(-1, falseArgs).wait());

}
TEST(Mock, Main) {
    SocketPair socketPair;
    ASSERT_TRUE(socketPair.good());
    PmsMock pms{socketPair.fd0()};
    auto fdStr = std::to_string(socketPair.fd1());
    const char * args[] = {"src/PlantowerPmsLinux", "-d", fdStr.c_str(), nullptr};
    Process child = Process::runCommand(socketPair.fd1(), args);
    clog << "Executed" << endl;
    clog << "PID: " << child.pid() << endl;

    clog << "status: " << child.wait() << endl;
    //    PmsMock mock{socketPair.fd0()};
    //    std::thread th{[&]{
    //            mock.run();
    //                   }};




}
