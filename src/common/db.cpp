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

#include <buffer.h>

std::string getSqlType(FieldType type)
{
    switch (type)
    {
    case FieldType::String:
        return "TEXT";
    case FieldType::Integer:
        return "INTEGER";
    case FieldType::Float:
        return "REAL";
    default:
        assert(false);
    }
    return "";
}

void table::load(const buffer &b)
{
    READ(b, id);
    READ_STRING(b, name);
    READ(b, unk4);
}

void field::load(const buffer &b)
{
    if (b.eof())
        return;
    READ(b, table_id);
    READ(b, id);
    READ_STRING(b, name);
    READ(b, type);
}

void tab::load(const buffer &b)
{
    READ(b, number_of_tables);
    READ(b, number_of_fields);

    auto n = number_of_tables;
    while (n--)
    {
        table t;
        t.load(b);
        tables[t.id] = t;
    }

    n = number_of_fields;
    while (n--)
    {
        field t;
        t.load(b);
        if (t.table_id == -1)
            continue;
        fields[t.id] = t;
    }
}

void value::load_index(const buffer &b)
{
    READ(b, table_id);
    READ_STRING(b, name);
    READ(b, offset);
    READ(b, data_size);
}

void value::load_fields(const tab &tab, buffer &b)
{
    buffer data(b, data_size, offset);
    while (!data.eof())
    {
        field_value fv;
            READ(data, fv.field_id);
        READ(data, fv.size);
        auto i = tab.fields.find(fv.field_id);
        if (i == tab.fields.end())
            continue;

        buffer data2(data, fv.size);
        switch (i->second.type)
        {
        case FieldType::String:
            fv.s.resize(fv.size);
            READ_N(data2, fv.s[0], fv.s.size());
            while (!fv.s.empty() && fv.s.back() == '\0')
                fv.s.resize(fv.s.size() - 1);
            break;
        case FieldType::Integer:
            if (sizeof(fv.i) <= fv.size)
                READ(data2, fv.i);
            else
                std::cout << "small int field: " << fv.size << "\n";
            break;
        case FieldType::Float:
            if (sizeof(fv.f) <= fv.size)
                READ(data2, fv.f);
            else
                std::cout << "small float field: " << fv.size << "\n";
            break;
        default:
            assert(false);
        }
        fields.push_back(fv);
    }
}

void db::load(const buffer &b)
{
    READ(b, number_of_values);

    auto n = number_of_values;
    while (n--)
    {
        value t;
        t.load_index(b);
        values.push_back(t);
    }
}

void db::open(const path &p)
{
    t.load(buffer(read_file(path(p) += ".tab")));
    load(buffer(read_file(path(p) += ".ind")));
    buffer b(read_file(path(p) += ".dat"));
    for (auto &v : values)
        v.load_fields(t, b);
}
