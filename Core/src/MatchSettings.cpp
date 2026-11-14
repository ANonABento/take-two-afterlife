#include "tta/MatchSettings.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace tta {
namespace {

struct Field {
    const char* key;
    int MatchSettings::*member;
    int min;
    int max;
};

const Field kFields[] = {
    {"rounds", &MatchSettings::rounds, 1, 10},
    {"buildSeconds", &MatchSettings::buildSeconds, 10, 600},
    {"runSeconds", &MatchSettings::runSeconds, 10, 600},
    {"editSeconds", &MatchSettings::editSeconds, 10, 600},
    {"introSeconds", &MatchSettings::introSeconds, 1, 60},
    {"ratingSeconds", &MatchSettings::ratingSeconds, 1, 60},
    {"revealSeconds", &MatchSettings::revealSeconds, 1, 60},
};

class Parser {
public:
    Parser(const std::string& text, const MatchSettings& base) : text_(text), base_(base) {}

    MatchSettings parse() {
        MatchSettings settings = base_;
        expect('{');
        if (peek() == '}') {
            ++pos_;
        } else {
            while (true) {
                const std::string key = parseString();
                expect(':');
                const long long value = parseInt();
                assign(settings, key, value);
                if (peek() == ',') { ++pos_; continue; }
                expect('}');
                break;
            }
        }
        skipSpace();
        if (pos_ != text_.size()) fail("trailing characters after object");
        return settings;
    }

private:
    [[noreturn]] void fail(const std::string& what) const {
        throw std::invalid_argument("settings JSON: " + what + " at offset " + std::to_string(pos_));
    }
    void skipSpace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    char peek() {
        skipSpace();
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }
    void expect(char c) {
        if (peek() != c) fail(std::string("expected '") + c + "'");
        ++pos_;
    }
    std::string parseString() {
        expect('"');
        std::string out;
        while (pos_ < text_.size() && text_[pos_] != '"') {
            if (text_[pos_] == '\\') fail("escapes are not supported in keys");
            out += text_[pos_++];
        }
        if (pos_ >= text_.size()) fail("unterminated string");
        ++pos_;
        return out;
    }
    long long parseInt() {
        skipSpace();
        const size_t start = pos_;
        if (pos_ < text_.size() && text_[pos_] == '-') ++pos_;
        const size_t digits = pos_;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        if (pos_ == digits || pos_ - digits > 9) fail("expected an integer");
        if (pos_ < text_.size() && (text_[pos_] == '.' || text_[pos_] == 'e' || text_[pos_] == 'E')) fail("expected an integer");
        return std::stoll(text_.substr(start, pos_ - start));
    }
    void assign(MatchSettings& settings, const std::string& key, long long value) {
        for (const Field& f : kFields) {
            if (key != f.key) continue;
            if (value < f.min || value > f.max) {
                fail("'" + key + "' must be between " + std::to_string(f.min) + " and " + std::to_string(f.max));
            }
            settings.*f.member = static_cast<int>(value);
            return;
        }
        fail("unknown key '" + key + "'");
    }

    const std::string& text_;
    const MatchSettings& base_;
    size_t pos_ = 0;
};

} // namespace

MatchSettings parseMatchSettings(const std::string& json, const MatchSettings& base) {
    return Parser(json, base).parse();
}

MatchSettings loadMatchSettings(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot read settings file: " + path);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parseMatchSettings(buffer.str());
}

std::string toJson(const MatchSettings& settings) {
    std::string out = "{";
    bool first = true;
    for (const Field& f : kFields) {
        if (!first) out += ", ";
        first = false;
        out += std::string("\"") + f.key + "\": " + std::to_string(settings.*f.member);
    }
    return out + "}";
}

} // namespace tta
