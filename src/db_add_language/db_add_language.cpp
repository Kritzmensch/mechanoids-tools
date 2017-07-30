/*
 * AIM db_extractor
 * Copyright (C) 2017 lzwdgc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../db_extractor/db.h"

#include <Polygon4/DataManager/Localization.h>
#include <Polygon4/DataManager/Storage.h>
#include <Polygon4/DataManager/Types.h>
#include <primitives/filesystem.h>
#include <primitives/executor.h>

#include <iostream>
#include <iomanip>
#include <numeric>

using namespace polygon4;
using namespace polygon4::detail;

struct string_index
{
    std::wstring s;
    IdType i = -1;

    string_index &operator=(const std::string &rhs)
    {
        s = string2wstring(str2utf8(rhs));
        return *this;
    }
};

using AimKV = std::unordered_map<std::string, string_index>;
using AimKVResolved = std::unordered_map<std::string, IdType>;
AimKVResolved kv_resolved;

template <class T>
int levenshtein_distance(const T &s1, const T &s2)
{
    // To change the type this function manipulates and returns, change
    // the return type and the types of the two variables below.
    int s1len = s1.size();
    int s2len = s2.size();

    auto column_start = (decltype(s1len))1;

    auto column = new decltype(s1len)[s1len + 1];
    std::iota(column + column_start, column + s1len + 1, column_start);

    for (auto x = column_start; x <= s2len; x++) {
        column[0] = x;
        auto last_diagonal = x - column_start;
        for (auto y = column_start; y <= s1len; y++) {
            auto old_diagonal = column[y];
            auto possibilities = {
                column[y] + 1,
                column[y - 1] + 1,
                last_diagonal + (s1[y - 1] == s2[x - 1] ? 0 : 1)
            };
            column[y] = std::min(possibilities);
            last_diagonal = old_diagonal;
        }
    }
    auto result = column[s1len];
    delete[] column;
    return result;
}

auto open(const path &p)
{
    db db;
    if (fs::exists(p / "quest.dat"))
        db.open(p / "quest");
    return db;
};

AimKV get_kv(const db &db)
{
    auto iter_tbl = std::find_if(db.t.tables.begin(), db.t.tables.end(), [](auto &t) {
        return t.second.name == "INFORMATION";
    });
    if (iter_tbl == db.t.tables.end())
        throw std::runtime_error("Table INFORMATION was not found");

    auto find_field = [&db, &iter_tbl](const std::string &name)
    {
        auto i = std::find_if(db.t.fields.begin(), db.t.fields.end(), [&iter_tbl, &name](auto &t) {
            return t.second.table_id == iter_tbl->second.id && t.second.name == name;
        });
        if (i == db.t.fields.end())
            throw std::runtime_error("Field " + name + " was not found");
        return i->first;
    };
    auto nid = find_field("NAME");
    auto tid = find_field("TEXT");

    AimKV kv;
    for (auto &v : db.values)
    {
        if (v.table_id != iter_tbl->second.id || v.name.empty())
            continue;
        for (auto &f : v.fields)
        {
            if ((f.field_id == nid || f.field_id == tid) && !f.s.empty())
                kv[v.name] = f.s;
        }
    }
    return kv;
}

AimKVResolved get_kv_resolved(const path &d, const Storage &storage)
{
    static const auto fn = "kv.resolved";

    AimKVResolved mres;
    if (fs::exists(fn))
    {
        std::ifstream f(fn);
        std::string s;
        IdType i;
        while (f)
        {
            f >> std::quoted(s);
            if (!f)
                break;
            f >> i;
            mres[s] = i;
        }
    }
    else
    {
        auto db1 = open(d / "ru" / "aim1");
        auto db2 = open(d / "ru" / "aim2");

        auto kv1 = get_kv(db1);
        auto kv2 = get_kv(db2);
        kv1.insert(kv2.begin(), kv2.end());
        auto sz = kv1.size();
        std::cout << "total kvs: " << sz << "\n";

        Executor e;
        int i = 0;
        for (auto &kv : kv1)
        {
            e.push([&storage, &i, &sz, &kv]()
            {
                std::cout << "total kvs: " << ++i << "/" << sz << "\n";
                std::map<int, IdType> m;
                for (auto &s : storage.strings)
                    m[levenshtein_distance<std::wstring>(kv.second.s, s.second->string.ru)] = s.first;
                if (m.empty())
                    return;
                kv.second.i = m.begin()->second;
            });
        }
        e.wait();

        std::ofstream f(fn);
        for (auto &kv : kv1)
        {
            mres[kv.first] = kv.second.i;
            f << std::quoted(kv.first) << " " << kv.second.i << "\n";
        }
    }
    return mres;
}

void process_lang(Storage &s, const path &p, polygon4::String polygon4::LocalizedString::*field)
{
    auto db1 = open(p);
    auto db2 = open(p / "aim1");
    auto db3 = open(p / "aim2");

    AimKV kvm;
    auto get_kv = [&kvm](auto &db)
    {
        AimKV kv1;
        if (db.number_of_values)
        {
            kv1 = ::get_kv(db);
            kvm.insert(kv1.begin(), kv1.end());
        }
    };
    get_kv(db1);
    get_kv(db2);
    get_kv(db3);

    std::string str;
    for (auto &kv : kvm)
    {
        auto i = kv_resolved.find(kv.first);
        if (i == kv_resolved.end())
            continue;
        auto &sold = s.strings[i->second]->string.*field;
        //sold = kv.second.s;
        str += "id: " + std::to_string(i->second) + "\n\n";
        str += "old:\n";
        str += wstring2string(sold) + "\n";
        str += "\n";
        str += "new:\n";
        str += wstring2string(kv.second.s) + "\n";
        str += "\n================================================\n\n";
    }
    write_file(p / (p.filename().string() + "_diff.txt"), str);
}

int main(int argc, char *argv[])
try
{
    if (argc != 3)
    {
        std::cout << "Usage: prog db.sqlite dir_to_lang_dbs" << "\n";
        return 1;
    }
    path d = argv[2];

    auto storage = initStorage(argv[1]);
    storage->load();
    kv_resolved = get_kv_resolved(d, *storage.get());

    for (auto &f : boost::make_iterator_range(fs::directory_iterator(d), {}))
    {
        if (!fs::is_directory(f))
            continue;

        auto p = f.path();

        if (0);
#define ADD_LANGUAGE(l, n) else if (p.filename() == #l && p.filename() != "ru") \
    {process_lang(*storage.get(), p, &polygon4::LocalizedString::l);}
#include <Polygon4/DataManager/Languages.inl>
#undef ADD_LANGUAGE
        else
        {
            std::cerr << "No such lang: " << p.filename().string() << "\n";
            continue;
        }
    }

    return 0;
}
catch (std::exception &e)
{
    printf("error: %s\n", e.what());
    return 1;
}
catch (...)
{
    printf("error: unknown exception\n");
    return 1;
}