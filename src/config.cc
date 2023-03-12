#include <iostream>
#include <cmath>

#include "config.h"

void iftb::config::setNumChunks(uint16_t numChunks) {
    uint8_t nbits = ilogb(numChunks) + 1, nhd; 
    chunk_hex_digits = nbits / 4;
    if (nbits % 4)
        chunk_hex_digits++;
    makeChunkDirs();
}

bool iftb::config::unicodesForPreload(std::string &tag,
                                      std::set<uint32_t> &unicodes) {
    std::unique_ptr<group_wrapper> wg;
    bool has = false;

    unicodes.clear();
    for (auto &gi: group_info) {
        bool found = false;
        for (auto &s: gi.preloads) {
            if (s == tag) {
                found = true;
                break;
            }
        }
        if (!found)
            continue;
        has = true;
        if (verbosity())
            std::cerr << "Adding point group " << gi.name << std::endl;
        if (gi.is_ordered) {
            wg = std::make_unique<vec_wrapper>(ordered_point_groups[gi.index]);
        } else {
            wg = std::make_unique<set_wrapper>(point_groups[gi.index]);
        }
        uint32_t cp = -1;
        while (wg->next(cp))
            unicodes.insert(cp);
    }
    return has;
}


void iftb::config::load_points(YAML::Node n, iftb::wr_set &s) {
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

void iftb::config::load_ordered_points(YAML::Node n, std::vector<uint32_t> &v) {
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

void iftb::config::loadPointGroup(YAML::Node n, bool is_ordered) {
    point_group_info pgi;
    pgi.name = n["name"].Scalar();
    if (n["preloads"].IsSequence()) {
        auto ln = n["preloads"];
        for (int i = 0; i < ln.size(); i++)
            pgi.preloads.push_back(ln[i].Scalar());
    }
    pgi.is_ordered = is_ordered;
    if (is_ordered) {
        pgi.index = ordered_point_groups.size();
        std::vector<uint32_t> b;
        load_ordered_points(n["points"], b);
        ordered_point_groups.push_back(std::move(b));
    } else {
        pgi.index = point_groups.size();
        iftb::wr_set s;
        load_points(n["points"], s);
        s.subtract(used_points);
        used_points._union(s);
        point_groups.push_back(std::move(s));
    }
    group_info.push_back(std::move(pgi));
}

bool iftb::config::prepDir(std::filesystem::path &p, bool thrw) {
    std::string e;
    if (std::filesystem::exists(p)) {
        if (!std::filesystem::is_directory(p)) {
            e = "Error: Path '";
            e += pathPrefix;
            e += "' exists but is not a directory";
            return false;
        } else {
            return true;
        }
    }
    e = "Could not create directory '" ;
    e += p;
    e += "'";
    if (thrw) {
        if (!std::filesystem::create_directory(p))
            throw std::runtime_error(e);
    } else {
        try {
            if (!std::filesystem::create_directory(p)) {
                std::cerr << e << std::endl;
                return false;
            }
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            return false;
        }
    }
    return true;
}

int iftb::config::load(std::string p, bool is_default) {
    auto yc = YAML::LoadFile(p.c_str());

    load_points(yc["base_points"], base_points);
    used_points.copy(base_points);
    auto ordered = yc["ordered_point_sets"];
    for (int k = 0; k < ordered.size(); k++)
        loadPointGroup(ordered[k], true);
    auto unordered = yc["unordered_point_sets"];
    for (int k = 0; k < unordered.size(); k++)
        loadPointGroup(unordered[k], false);
    auto feat_sub_cut = yc["feature_subset_cutoff"];
    if (feat_sub_cut.IsScalar())
        feat_subset_cutoff = feat_sub_cut.as<uint32_t>();
    auto targ_chunk_sz = yc["target_chunk_size"];
    if (targ_chunk_sz.IsScalar())
        target_chunk_size = targ_chunk_sz.as<uint32_t>();

    if (verbosity() <= 2)
        return 0;

    std::cerr << "Config:" << std::endl;
    std::cerr << "  feature subsetting cutoff size: " << feat_subset_cutoff << std::endl;
    std::cerr << "  target chunk size: " << target_chunk_size << std::endl;
    std::cerr << "  base point population: " << base_points.size() << std::endl;
    std::cerr << "  total point population: " << used_points.size() << std::endl;
    std::cerr << "  # of ordered point groups: " << ordered_point_groups.size();
    std::cerr << " (";
    bool printed = false;
    for (auto &v: ordered_point_groups) {
        if (printed)
            std::cerr << ", ";
        printed = true;
        std::cerr << v.size();
    }
    std::cerr << ")" << std::endl;
    std::cerr << "  # of other point groups: " << point_groups.size();
    std::cerr << " (";
    printed = false;
    for (auto &v: point_groups) {
        if (printed)
            std::cerr << ", ";
        printed = true;
        std::cerr << v.size();
    }
    std::cerr << ")" << std::endl;
    return 0;
}
