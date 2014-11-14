/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "autopatchers/noobdev/noobdevpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>

#include "private/fileutils.h"


/*! \cond INTERNAL */
class NoobdevBasePatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
};
/*! \endcond */


static const std::string UpdaterScript =
        "META-INF/com/google/android/updater-script";
static const std::string DualBootSh = "dualboot.sh";
static const std::string BuildProp = "system/build.prop";

static const std::string PrintEmpty = "ui_print(\"\");";


NoobdevBasePatcher::NoobdevBasePatcher(const PatcherConfig * const pc,
                                       const FileInfo * const info)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
}

NoobdevBasePatcher::~NoobdevBasePatcher()
{
}

PatcherError NoobdevBasePatcher::error() const
{
    return PatcherError();
}

////////////////////////////////////////////////////////////////////////////////

const std::string NoobdevMultiBoot::Id = "NoobdevMultiBoot";

NoobdevMultiBoot::NoobdevMultiBoot(const PatcherConfig * const pc,
                                   const FileInfo* const info)
    : NoobdevBasePatcher(pc, info)
{
}

std::string NoobdevMultiBoot::id() const
{
    return Id;
}

std::vector<std::string> NoobdevMultiBoot::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> NoobdevMultiBoot::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(UpdaterScript);
    files.push_back(DualBootSh);
    return files;
}

bool NoobdevMultiBoot::patchFiles(const std::string &directory,
                                  const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // My ROM has built-in dual boot support, so we'll hack around that to
    // support triple, quadruple, ... boot

    // UpdaterScript begin
    FileUtils::readToMemory(directory + "/" + UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->find("system/bin/dualboot.sh") != std::string::npos) {
            // Remove existing dualboot.sh lines
            it = lines.erase(it);
        } else if (it->find("boot installation is") != std::string::npos) {
            // Remove confusing messages, but need to keep at least one
            // statement in the if-block to keep update-binary happy
            *it = PrintEmpty;
        } else if (it->find("set-secondary") != std::string::npos) {
            *it = PrintEmpty;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + UpdaterScript, contents);
    // UpdaterScript end

    // DualBootSh begin
    FileUtils::readToMemory(directory + "/" + DualBootSh, &contents);
    const std::string noDualBoot("echo 'ro.dualboot=0' > /tmp/dualboot.prop\n");
    contents.insert(contents.end(), noDualBoot.begin(), noDualBoot.end());
    FileUtils::writeFromMemory(directory + "/" + DualBootSh, contents);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string NoobdevSystemProp::Id = "NoobdevSystemProp";

NoobdevSystemProp::NoobdevSystemProp(const PatcherConfig * const pc,
                                     const FileInfo* const info)
    : NoobdevBasePatcher(pc, info)
{
}

std::string NoobdevSystemProp::id() const
{
    return Id;
}

std::vector<std::string> NoobdevSystemProp::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> NoobdevSystemProp::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(BuildProp);
    return files;
}

bool NoobdevSystemProp::patchFiles(const std::string &directory,
                                   const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // The auto-updater in my ROM needs to know if the ROM has been patched

    // BuildProp begin
    FileUtils::readToMemory(directory + "/" + BuildProp, &contents);
    const std::string prop = (boost::format("ro.chenxiaolong.patched=%1%\n")
            % m_impl->info->partConfig()->id()).str();
    contents.insert(contents.end(), prop.begin(), prop.end());
    FileUtils::writeFromMemory(directory + "/" + BuildProp, contents);
    // BuildProp end

    return true;
}
