#include "core/protocol-percentile.hpp"
#include "core/coda.hpp"
#include "core/file_util.hpp"
#include "core/literal.hpp"
#include "HElib/FHE.h"
#include "HElib/timing.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/dirent.h>
bool create_dir(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path.c_str(), 0777) != 0)
            return false;
    } else {
        auto files = util::listDir(path, util::flag_t::FILE_ONLY);
        for (auto file : files) {
            auto full = util::concatenate(path, file);
            if (full.compare(".") == 0 || full.compare("..")) continue;
            unlink(full.c_str());
        }

    }
    return true;
}

long _sizeP = 14;
long _sizeQ = 2;
struct Cell {
    long p, q, v;
};
// test cases
std::vector<Cell> _cells = {
    {1, 2, 1},
    {2, 1, 1},
    {14, 1, 2},
    {14, 2, 2}
};

bool gen_content(const std::string &path) {
    std::ofstream fout(util::concatenate(path, "data.csv"), std::ios::out | std::ios::trunc);
    if (!fout.is_open())
        return false;
    fout << "#3 " << _sizeP << " " << _sizeQ << " 2\n";
    for (auto &cell : _cells)
        fout << cell.p << " " << cell.q << " " << cell.v << "\n";
    fout.close();
    return true;
}

int check(const std::vector<std::string> &share_1,
          const std::vector<std::string> &share_2) {
    if (share_1.size() != share_2.size()) {
        std::cout << share_1.size() << "≠" << share_2.size() << "\n";
        return -1;
    }

    auto valuer = [](const std::string &s) -> long { return literal::stol(s); };
    std::vector<int> indices;
    for (auto cell : _cells) {
        int index = (cell.p - 1) * _sizeQ + cell.q - 1;
        if (valuer(share_1[index]) - valuer(share_2[index]) != 2 * cell.v)
            return -1;
        indices.push_back(index);
    }

    std::sort(indices.begin(), indices.end());
    for (int index = 0; index < _sizeP * _sizeQ; index++) {
        if (std::binary_search(indices.begin(), indices.end(), index))
            continue;
        if (valuer(share_1[index]) - valuer(share_2[index]) != 0)
            return -1;
    }
    std::cout << "passed\n";
    return 0;
}

int main () {
    if (!create_dir("test-ct-1"))
        return -1;
    if (!create_dir("test-ct-2"))
        return -1;

    core::context_ptr context = std::make_shared<FHEcontext>(2048, 8191, 3);
    buildModChain(*context, 5);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    PercentileProtocol protocol(10);
    gen_content("test-ct-1");
    if (!protocol.encrypt("test-ct-1/data.csv", "test-ct-1/", true, pk, context))
        return -1;
    gen_content("test-ct-2");
    if (!protocol.encrypt("test-ct-2/data.csv", "test-ct-2/", true, pk, context))
        return -1;
    std::vector<std::string> inputDirs;
    inputDirs.push_back("test-ct-1");
    inputDirs.push_back("test-ct-2");

    if (!create_dir("test-out"))
       return -1;
    std::cout << "to eval\n";
    if (!protocol.evaluate(inputDirs, "test-out", {"2"}, pk, context))
        return -1;
    std::cout << "to dec\n";
    if (!protocol.decrypt("test-out/" + core::core_setting.resulting_file,
                    "./", pk, sk, context))
        return -1;

    // std::ifstream fin(core::core_setting.decrypted_file);
    // if (!fin.is_open())
    //     return -1;
    // std::ifstream fin2("test-out/" + core::core_setting.random_share_file);
    // if (!fin2.is_open())
    //     return -1;
    //
    // std::string line1, line2;
    // std::getline(fin, line1);
    // std::getline(fin2, line2);
    // fin.close();
    // fin2.close();
    // return check(util::splitBySpace(line1), util::splitBySpace(line2));
}
