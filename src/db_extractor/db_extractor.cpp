/*
 * AIM db_extractor
 * Copyright (C) 2015 lzwdgc
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

#include "db.h"

#include <fstream>

#include <Windows.h>

void open_tab(string path, tab &tab)
{
    path += ".tab";
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return;
    tab.load(f);
    fclose(f);
}

void open_index(string path, db &index)
{
    path += ".ind";
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return;
    index.load(f);
    fclose(f);
}

void open_data(string path, db &index)
{
    path += ".dat";
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return;
    for (auto &v : index.values)
        v.load_data(f);
    fclose(f);
}

void open_db(string path, db &db)
{
    open_tab(path, db.t);
    open_index(path, db);
    open_data(path, db);
    for (auto &v : db.values)
        v.extract_fields(db.t);
}

string str2utf8(string codepage_str)
{
    int size = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, codepage_str.c_str(),
                                   codepage_str.length(), nullptr, 0);
    std::wstring utf16_str(size, '\0');
    MultiByteToWideChar(CP_ACP, MB_COMPOSITE, codepage_str.c_str(),
                        codepage_str.length(), &utf16_str[0], size);

    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(),
                                        utf16_str.length(), nullptr, 0,
                                        nullptr, nullptr);
    std::string utf8_str(utf8_size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(),
                        utf16_str.length(), &utf8_str[0], utf8_size,
                        nullptr, nullptr);
    return utf8_str;
}

void create_sql(string path, const db &db)
{
    ofstream ofile(path + ".sql");
    if (!ofile)
        return;

    const string id = "ID";
    const string row_type = "TEXT_ID";
    for (auto &table : db.t.tables)
    {
        auto &t = table.second;
        string name = str2utf8(t.name);
        ofile << "drop table if exists " << name << ";\n";
        ofile << "create table \"" << name << "\"\n";
        ofile << "(\n";
        string s;
        s += "  \"" + str2utf8(id) + "\" INTEGER,\n";
        s += "  \"" + str2utf8(row_type) + "\" TEXT,\n";
        for (auto &f : db.t.fields)
        {
            if (f.second.table_id != t.id)
                continue;
            s += "  \"" + str2utf8(f.second.name) + "\" " + getSqlType(f.second.type) + ",\n";
        }
        s.resize(s.size() - 2);
        ofile << s << "\n";
        ofile << ");\n";
        ofile << "\n";
    }

    map<int,int> idx;
    for (auto &v : db.values)
    {
        auto tbl = db.t.tables.find(v.table_id);
        if (tbl == db.t.tables.end())
            continue;
        ofile << "insert into \"" << str2utf8(tbl->second.name) << "\" (";
        string s;
        s += "'" + str2utf8(id) + "', ";
        s += "'" + str2utf8(row_type) + "', ";
        for (auto &f : v.fields)
        {
            auto fld = db.t.fields.find(f.field_id);
            s += "'" + str2utf8(fld->second.name) + "', ";
        }
        s.resize(s.size() - 2);
        ofile << s << ") values (";
        s.clear();
        s += "'" + to_string(idx[v.table_id]++) + "', ";
        s += "'" + str2utf8(v.name) + "', ";
        for (auto &f : v.fields)
        {
            s += "'";
            auto fld = db.t.fields.find(f.field_id);
            switch (fld->second.type)
            {
                case T_STRING:
                    s += str2utf8(f.s.c_str());
                    break;
                case T_INTEGER:
                    s += to_string(f.i);
                    break;
                case T_FLOAT:
                    s += to_string(f.f);
                    break;
                default:
                    assert(false);
            }
            s += "', ";
        }
        s.resize(s.size() - 2);
        ofile << s << ");\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage:\n" << argv[0] << " path/to/aim_game/data/db" << "\n" << argv[0] << " path/to/aim_game/data/quest" << "\n";
        return 1;
    }
    string path = argv[1];
    db db;
    open_db(path, db);
    create_sql(path, db);
    return 0;
}