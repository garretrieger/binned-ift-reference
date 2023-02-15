#include <iostream>

#include "config.h"

void config::load_points(YAML::Node n, set &s) {
    for (int k = 0; k < n.size(); k++) {
        if (n[k].IsScalar())
            s.add(n[k].as<uint32_t>());
        else if (n[k].IsSequence()) {
            if (n[k].size() != 2) {
                throw YAML::Exception(n[k].Mark(), "Unicode ranges must have two values");
            }
            s.add_range(n[k][0].as<uint32_t>(), n[k][1].as<uint32_t>());
        } else
            throw YAML::Exception(n[k].Mark(), "Point must be an integer or a two-integer range sequence");
    }
}

void config::load_ordered_points(YAML::Node n, std::vector<uint32_t> &v) {
    for (int k = 0; k < n.size(); k++) {
        if (n[k].IsScalar()) {
            uint32_t p = n[k].as<uint32_t>();
            if (!used_points.has(p)) {
                v.push_back(p);
                used_points.add(p);
            }
        } else
            throw YAML::Exception(n[k].Mark(), "Point must be an integer");
    }
}

void config::load(const char *cfname) {
    auto yc = YAML::LoadFile(cfname);

    load_points(yc["base_points"], base_points);
    used_points.copy(base_points);
    auto ordered = yc["ordered_point_sets"];
    for (int k = 0; k < ordered.size(); k++) {
        std::vector<uint32_t> b;
        load_ordered_points(ordered[k], b);
        ordered_point_groups.push_back(std::move(b));
    }
    auto unordered = yc["unordered_point_sets"];
    for (int k = 0; k < unordered.size(); k++) {
        set s;
        load_points(unordered[k], s);
        s.subtract(used_points);
        used_points._union(s);
        point_groups.push_back(std::move(s));
    }
    auto feat_sub_cut = yc["feature_subset_cutoff"];
    if (feat_sub_cut.IsScalar())
        feat_subset_cutoff = feat_sub_cut.as<uint32_t>();
    auto targ_chunk_sz = yc["target_chunk_size"];
    if (targ_chunk_sz.IsScalar())
        target_chunk_size = targ_chunk_sz.as<uint32_t>();
    auto outdir = yc["output_dir"];
    if (outdir.IsScalar())
        output_dir = outdir.as<std::string>();

    std::cerr << "Config:" << std::endl;
    std::cerr << "  feature subsetting cutoff size: " << feat_subset_cutoff << std::endl;
    std::cerr << "  target chunk size: " << target_chunk_size << std::endl;
    std::cerr << "  base point population: " << base_points.size() << std::endl;
    std::cerr << "  total point population: " << used_points.size() << std::endl;
    std::cerr << "  # of ordered point groups: " << ordered_point_groups.size();
    std::cerr << " (";
    for (auto v: ordered_point_groups)
        std::cerr << v.size() << ",";
    std::cerr << ")" << std::endl;
    std::cerr << "  # of other point groups: " << point_groups.size();
    std::cerr << " (";
    for (auto &v: point_groups)
        std::cerr << v.size() << ",";
    std::cerr << ")" << std::endl;
}
