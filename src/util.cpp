#include "util.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace util {

std::string recortar(std::string s) {
  const auto no_espacio = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), no_espacio));
  s.erase(std::find_if(s.rbegin(), s.rend(), no_espacio).base(), s.end());
  return s;
}

std::string a_minusculas(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string sin_separadores(std::string s) {
  std::string salida;
  for (char c : s) {
    if (c == ' ' || c == '_' || c == ',' || c == '\'' || c == '\t') continue;
    salida.push_back(c);
  }
  return salida;
}

std::string normalizar(std::string s) {
  // Acentos y enie en UTF-8 ocupan dos bytes: los mapeamos a su letra base.
  static const std::pair<const char*, char> kAcentos[] = {
      {"\xc3\xa1", 'a'}, {"\xc3\xa9", 'e'}, {"\xc3\xad", 'i'}, {"\xc3\xb3", 'o'},
      {"\xc3\xba", 'u'}, {"\xc3\xbc", 'u'}, {"\xc3\xb1", 'n'}, {"\xc3\x81", 'a'},
      {"\xc3\x89", 'e'}, {"\xc3\x8d", 'i'}, {"\xc3\x93", 'o'}, {"\xc3\x9a", 'u'},
      {"\xc3\x91", 'n'},
  };
  std::string sin_tildes;
  for (std::size_t i = 0; i < s.size();) {
    bool reemplazado = false;
    for (const auto& par : kAcentos) {
      const std::string clave = par.first;
      if (s.compare(i, clave.size(), clave) == 0) {
        sin_tildes.push_back(par.second);
        i += clave.size();
        reemplazado = true;
        break;
      }
    }
    if (!reemplazado) sin_tildes.push_back(s[i++]);
  }

  std::string salida;
  for (unsigned char c : a_minusculas(sin_tildes)) {
    if (std::isalnum(c)) salida.push_back(static_cast<char>(c));
    else if (!salida.empty() && salida.back() != ' ') salida.push_back(' ');
  }
  return recortar(salida);
}

std::optional<unsigned long long> parsear_bits(const std::string& s) {
  const std::string limpio = sin_separadores(recortar(s));
  if (limpio.empty() || limpio.size() > 64) return std::nullopt;
  unsigned long long v = 0;
  for (char c : limpio) {
    if (c != '0' && c != '1') return std::nullopt;
    v = (v << 1) | static_cast<unsigned long long>(c - '0');
  }
  return v;
}

std::optional<long long> parsear_entero(const std::string& s) {
  std::string t = a_minusculas(sin_separadores(recortar(s)));
  if (t.empty()) return std::nullopt;

  bool negativo = false;
  if (t[0] == '+' || t[0] == '-') {
    negativo = (t[0] == '-');
    t.erase(t.begin());
  }
  if (t.empty()) return std::nullopt;

  int base = 10;
  if (t.size() > 2 && t[0] == '0' && t[1] == 'x') {
    base = 16;
    t = t.substr(2);
  } else if (t.size() > 2 && t[0] == '0' && t[1] == 'b') {
    base = 2;
    t = t.substr(2);
  }

  unsigned long long v = 0;
  for (char c : t) {
    int digito;
    if (c >= '0' && c <= '9') digito = c - '0';
    else if (c >= 'a' && c <= 'f') digito = c - 'a' + 10;
    else return std::nullopt;
    if (digito >= base) return std::nullopt;
    v = v * static_cast<unsigned long long>(base) + static_cast<unsigned long long>(digito);
  }
  const long long con_signo = static_cast<long long>(v);
  return negativo ? -con_signo : con_signo;
}

std::optional<unsigned long long> parsear_direccion(const std::string& s) {
  std::string t = a_minusculas(sin_separadores(recortar(s)));
  if (t.size() > 2 && t[0] == '0' && t[1] == 'x') t = t.substr(2);
  if (t.empty() || t.size() > 16) return std::nullopt;
  unsigned long long v = 0;
  for (char c : t) {
    int digito;
    if (c >= '0' && c <= '9') digito = c - '0';
    else if (c >= 'a' && c <= 'f') digito = c - 'a' + 10;
    else return std::nullopt;
    v = v * 16u + static_cast<unsigned long long>(digito);
  }
  return v;
}

std::string a_binario(unsigned long long v, int bits) {
  std::string salida;
  for (int i = bits - 1; i >= 0; --i) salida.push_back(((v >> i) & 1ull) ? '1' : '0');
  return salida;
}

std::string a_hex(unsigned long long v, int digitos) {
  std::ostringstream os;
  os << std::hex;
  os << v;
  std::string s = os.str();
  while (static_cast<int>(s.size()) < digitos) s.insert(s.begin(), '0');
  return "0x" + s;
}

}  // namespace util
