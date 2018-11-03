#include <gtest/gtest.h>
#include "Properties.h"

TEST(Properties, Main) {
    Properties props("/etc/ufw/ufw.conf");
    EXPECT_EQ("no", props.getString("ENABLED"));
    EXPECT_EQ("xxx", props.getString("MissingElement", "xxx"));
    EXPECT_THROW(props.getString("MissingElement"), Properties::KeyNotFound);
}

TEST(Pms, Main) {
    EXPECT_EQ(0, system("src/PlantowerPmsLinux -h"));
}
