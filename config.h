#include <vector>

#include "yaml-cpp/yaml.h"

#include "wrappers.h"

#pragma once

struct config {
    set base_points, other_points, used_points;
    std::vector<std::vector<uint32_t>> ordered_point_groups;
    std::vector<set> point_groups;
    void load_points(YAML::Node n, set &s);
    void load_ordered_points(YAML::Node n, std::vector<uint32_t> &s);
    void load(const char *cfname = "config.yaml");
};
