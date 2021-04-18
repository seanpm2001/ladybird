/*
 * Copyright (c) 2021, the SerenityOS developers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <LibCrypto/ASN1/ASN1.h>

namespace Crypto::ASN1 {

String kind_name(Kind kind)
{
    switch (kind) {
    case Kind::Eol:
        return "EndOfList";
    case Kind::Boolean:
        return "Boolean";
    case Kind::Integer:
        return "Integer";
    case Kind::BitString:
        return "BitString";
    case Kind::OctetString:
        return "OctetString";
    case Kind::Null:
        return "Null";
    case Kind::ObjectIdentifier:
        return "ObjectIdentifier";
    case Kind::IA5String:
        return "IA5String";
    case Kind::PrintableString:
        return "PrintableString";
    case Kind::Utf8String:
        return "UTF8String";
    case Kind::UTCTime:
        return "UTCTime";
    case Kind::GeneralizedTime:
        return "GeneralizedTime";
    case Kind::Sequence:
        return "Sequence";
    case Kind::Set:
        return "Set";
    }

    return "InvalidKind";
}

String class_name(Class class_)
{
    switch (class_) {
    case Class::Application:
        return "Application";
    case Class::Context:
        return "Context";
    case Class::Private:
        return "Private";
    case Class::Universal:
        return "Universal";
    }

    return "InvalidClass";
}

String type_name(Type type)
{
    switch (type) {
    case Type::Constructed:
        return "Constructed";
    case Type::Primitive:
        return "Primitive";
    }

    return "InvalidType";
}

Optional<Core::DateTime> parse_utc_time(const StringView& time)
{
    // YYMMDDhhmm[ss]Z or YYMMDDhhmm[ss](+|-)hhmm
    GenericLexer lexer(time);
    auto year_in_century = lexer.consume(2).to_uint();
    auto month = lexer.consume(2).to_uint();
    auto day = lexer.consume(2).to_uint();
    auto hour = lexer.consume(2).to_uint();
    auto minute = lexer.consume(2).to_uint();
    Optional<unsigned> seconds, offset_hours, offset_minutes;
    [[maybe_unused]] bool negative_offset = false;
    if (!lexer.next_is('Z')) {
        if (!lexer.next_is(is_any_of("+-"))) {
            seconds = lexer.consume(2).to_uint();
            if (!seconds.has_value()) {
                return {};
            }
        }

        if (lexer.next_is(is_any_of("+-"))) {
            negative_offset = lexer.consume() == '-';
            offset_hours = lexer.consume(2).to_uint();
            offset_minutes = lexer.consume(2).to_uint();
            if (!offset_hours.has_value() || !offset_minutes.has_value()) {
                return {};
            }
        } else {
            lexer.consume();
        }
    } else {
        lexer.consume();
    }

    if (!year_in_century.has_value() || !month.has_value() || !day.has_value() || !hour.has_value() || !minute.has_value()) {
        return {};
    }

    auto full_year = (Core::DateTime::now().year() / 100) * 100 + year_in_century.value();
    auto full_seconds = seconds.value_or(0);

    // FIXME: Handle offsets!
    if (offset_hours.has_value() || offset_minutes.has_value())
        dbgln("FIXME: Implement UTCTime with offset!");

    return Core::DateTime::create(full_year, month.value(), day.value(), hour.value(), minute.value(), full_seconds);
}

Optional<Core::DateTime> parse_generalized_time(const StringView& time)
{
    // YYYYMMDDhh[mm[ss[.fff]]] or YYYYMMDDhh[mm[ss[.fff]]]Z or YYYYMMDDhh[mm[ss[.fff]]](+|-)hhmm
    GenericLexer lexer(time);
    auto year = lexer.consume(4).to_uint();
    auto month = lexer.consume(2).to_uint();
    auto day = lexer.consume(2).to_uint();
    auto hour = lexer.consume(2).to_uint();
    Optional<unsigned> minute, seconds, miliseconds, offset_hours, offset_minutes;
    [[maybe_unused]] bool negative_offset = false;
    if (!lexer.is_eof()) {
        if (lexer.consume_specific('Z'))
            goto done_parsing;

        if (!lexer.next_is(is_any_of("+-"))) {
            minute = lexer.consume(2).to_uint();
            if (!minute.has_value()) {
                return {};
            }
            if (lexer.consume_specific('Z'))
                goto done_parsing;
        }

        if (!lexer.next_is(is_any_of("+-"))) {
            seconds = lexer.consume(2).to_uint();
            if (!seconds.has_value()) {
                return {};
            }
            if (lexer.consume_specific('Z'))
                goto done_parsing;
        }

        if (lexer.consume_specific('.')) {
            miliseconds = lexer.consume(3).to_uint();
            if (!miliseconds.has_value()) {
                return {};
            }
            if (lexer.consume_specific('Z'))
                goto done_parsing;
        }

        if (lexer.next_is(is_any_of("+-"))) {
            negative_offset = lexer.consume() == '-';
            offset_hours = lexer.consume(2).to_uint();
            offset_minutes = lexer.consume(2).to_uint();
            if (!offset_hours.has_value() || !offset_minutes.has_value()) {
                return {};
            }
        } else {
            lexer.consume();
        }
    }

done_parsing:;

    if (!year.has_value() || !month.has_value() || !day.has_value() || !hour.has_value()) {
        return {};
    }

    // FIXME: Handle offsets!
    if (offset_hours.has_value() || offset_minutes.has_value())
        dbgln("FIXME: Implement GeneralizedTime with offset!");

    // Unceremonially drop the miliseconds on the floor.
    return Core::DateTime::create(year.value(), month.value(), day.value(), hour.value(), minute.value_or(0), seconds.value_or(0));
}

}
