/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "libdnf-cli/output/changelogs.hpp"

#include "libdnf-cli/tty.hpp"

#include "libdnf/rpm/package_set.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>


namespace libdnf::cli::output {

void print_changelogs(
    libdnf::rpm::PackageQuery & query,
    libdnf::rpm::PackageQuery & full_package_query,
    bool upgrades,
    int32_t count,
    int64_t since) {
    // by_srpm
    std::map<std::string, std::vector<libdnf::rpm::Package>> by_srpm;
    for (auto pkg : query) {
        by_srpm[pkg.get_source_name() + '/' + pkg.get_evr()].push_back(pkg);
    }

    libdnf::rpm::PackageQuery installed(full_package_query);
    installed.filter_installed();

    for (auto & [name, packages] : by_srpm) {
        // Print header
        std::cout << "Changelogs for ";
        std::vector<std::string> nevras;
        nevras.reserve(packages.size());
        for (auto & pkg : packages) {
            nevras.push_back(pkg.get_nevra());
        }
        std::sort(nevras.begin(), nevras.end());
        std::cout << nevras[0];
        for (size_t idx = 1; idx < nevras.size(); ++idx) {
            std::cout << ", " << nevras[idx];
        }
        std::cout << std::endl;

        auto changelogs = packages[0].get_changelogs();
        std::sort(
            changelogs.begin(),
            changelogs.end(),
            [](const libdnf::rpm::Changelog & a, const libdnf::rpm::Changelog & b) {
                return a.timestamp > b.timestamp;
            });

        // filter changelog
        if (upgrades) {
            // Find the newest changelog on the installed version of the
            // package, and filter out any changelogs newer than that
            libdnf::rpm::PackageQuery query(installed);
            query.filter_name({packages[0].get_name()});
            time_t newest_timestamp = 0;
            for (auto pkg : query) {
                for (auto & chlog : pkg.get_changelogs()) {
                    if (chlog.timestamp > newest_timestamp) {
                        newest_timestamp = chlog.timestamp;
                    }
                }
            }
            size_t idx;
            for (idx = 0; idx < changelogs.size() && changelogs[idx].timestamp > newest_timestamp; ++idx) {
            }
            changelogs.erase(changelogs.begin() + static_cast<int>(idx), changelogs.end());
        } else if (count != 0) {
            if (count > 0) {
                if (static_cast<size_t>(count) < changelogs.size()) {
                    changelogs.erase(changelogs.begin() + count, changelogs.end());
                }
            } else {
                if (static_cast<size_t>(-count) < changelogs.size()) {
                    changelogs.erase(changelogs.end() + count, changelogs.end());
                } else {
                    changelogs.clear();
                }
            }
        } else if (since > 0) {
            size_t idx;
            for (idx = 0; idx < changelogs.size() && changelogs[idx].timestamp >= since; ++idx) {
            }
            changelogs.erase(changelogs.begin() + static_cast<int>(idx), changelogs.end());
        }

        for (auto & chlog : changelogs) {
            std::cout << std::put_time(std::gmtime(&chlog.timestamp), "* %a %b %d %X %Y ");
            std::cout << chlog.author << "\n";
            std::cout << chlog.text << "\n" << std::endl;
        }
    }
}

}  // namespace libdnf::cli::output
