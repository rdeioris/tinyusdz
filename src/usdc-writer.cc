#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>  // include API for expanding a file path
#endif


#include "usdc-writer.hh"
#include "crate-format.hh"
#include "io-util.hh"
#include "lz4-compression.hh"
#include "token-type.hh"

#include <fstream>
#include <iostream>
#include <sstream>

#ifdef TINYUSDZ_PRODUCTION_BUILD
// Do not include full filepath for privacy.
#define PUSH_ERROR(s) { \
  std::ostringstream ss; \
  ss << "[usdc-writer] " << __func__ << "():" << __LINE__ << " "; \
  ss << s; \
  err_ += ss.str() + "\n"; \
} while (0)

#if 0
#define PUSH_WARN(s) { \
  std::ostringstream ss; \
  ss << "[usdc-writer] " << __func__ << "():" << __LINE__ << " "; \
  ss << s; \
  warn_ += ss.str() + "\n"; \
} while (0)
#endif
#else
#define PUSH_ERROR(s) { \
  std::ostringstream ss; \
  ss << __FILE__ << ":" << __func__ << "():" << __LINE__ << " "; \
  ss << s; \
  err_ += ss.str() + "\n"; \
} while (0)

#if 0
#define PUSH_WARN(s) { \
  std::ostringstream ss; \
  ss << __FILE__ << ":" << __func__ << "():" << __LINE__ << " "; \
  ss << s; \
  warn_ += ss.str() + "\n"; \
} while (0)
#endif
#endif

#ifndef TINYUSDZ_PRODUCTION_BUILD
#define TINYUSDZ_LOCAL_DEBUG_PRINT
#endif

#if defined(TINYUSDZ_LOCAL_DEBUG_PRINT)
#define DCOUT(x) do { std::cout << __FILE__ << ":" << __func__ << ":" << std::to_string(__LINE__) << " " << x << "\n"; } while (false)
#else
#define DCOUT(x) do { (void)(x); } while(false)
#endif

namespace tinyusdz {
namespace usdc {

namespace {

constexpr size_t kSectionNameMaxLength = 15;

#ifdef _WIN32
std::wstring UTF8ToWchar(const std::string &str) {
  int wstr_size =
      MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), nullptr, 0);
  std::wstring wstr(size_t(wstr_size), 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), &wstr[0],
                      int(wstr.size()));
  return wstr;
}

std::string WcharToUTF8(const std::wstring &wstr) {
  int str_size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.size()),
                                     nullptr, 0, nullptr, nullptr);
  std::string str(size_t(str_size), 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.size()), &str[0],
                      int(str.size()), nullptr, nullptr);
  return str;
}
#endif


struct Section {
  Section() { memset(this, 0, sizeof(*this)); }
  Section(char const *name, int64_t start, int64_t size);
  char name[kSectionNameMaxLength + 1];
  int64_t start, size;  // byte offset to section info and its data size
};

//
// TOC = list of sections.
//
struct TableOfContents {
  // Section const *GetSection(SectionName) const;
  // int64_t GetMinimumSectionStart() const;
  std::vector<Section> sections;
};

struct Field {
  // FIXME(syoyo): Do we need 4 bytes padding as done in pxrUSD?
  // uint32_t padding_;

  crate::TokenIndex token_index;
  crate::ValueRep value_rep;
};

class Packer {
 public:

  crate::TokenIndex AddToken(const Token &token);
  crate::StringIndex AddString(const std::string &str);
  crate::PathIndex AddPath(const Path &path);
  crate::FieldIndex AddField(const Field &field);
  crate::FieldSetIndex AddFieldSet(const std::vector<crate::FieldIndex> &field_indices);

 private:

  // TODO: Custom Hasher
  std::unordered_map<Token, crate::TokenIndex> token_to_index_map;
  std::unordered_map<std::string, crate::StringIndex> string_to_index_map;
  std::unordered_map<Path, crate::PathIndex> path_to_index_map;
  std::unordered_map<Field, crate::FieldIndex> field_to_index_map;
  std::unordered_map<std::vector<crate::FieldIndex>, crate::FieldSetIndex> fieldset_to_index_map;

  std::vector<Token> tokens_;
  std::vector<std::string> strings_;
  std::vector<Path> paths_;
  std::vector<Field> fields_;
  std::vector<crate::FieldIndex> fieldsets_; // flattened 1D array of FieldSets. Each span is terminated by Index()(= ~0)

};

crate::TokenIndex Packer::AddToken(const Token &token) {
  if (token_to_index_map.count(token)) {
    return token_to_index_map[token];
  }

  // index = size of umap
  token_to_index_map[token] = crate::TokenIndex(uint32_t(tokens_.size()));
  tokens_.emplace_back(token);

  return token_to_index_map[token];
}

crate::StringIndex Packer::AddString(const std::string &str) {
  if (string_to_index_map.count(str)) {
    return string_to_index_map[str];
  }

  // index = size of umap
  string_to_index_map[str] = crate::StringIndex(uint32_t(strings_.size()));
  strings_.emplace_back(str);

  return string_to_index_map[str];
}

crate::PathIndex Packer::AddPath(const Path &path) {
  if (path_to_index_map.count(path)) {
    return path_to_index_map[path];
  }

  // index = size of umap
  path_to_index_map[path] = crate::PathIndex(uint32_t(paths_.size()));
  paths_.emplace_back(path);

  return path_to_index_map[path];
}

crate::FieldIndex Packer::AddField(const Field &field) {
  if (field_to_index_map.count(field)) {
    return field_to_index_map[field];
  }

  // index = size of umap
  field_to_index_map[field] = crate::FieldIndex(uint32_t(fields_.size()));
  fields_.emplace_back(field);

  return field_to_index_map[field];
}

crate::FieldSetIndex Packer::AddFieldSet(const std::vector<crate::FieldIndex> &fieldset) {
  if (fieldset_to_index_map.count(fieldset)) {
    return fieldset_to_index_map[fieldset];
  }

  // index = size of umap = star index of FieldSet span.
  fieldset_to_index_map[fieldset] = crate::FieldSetIndex(uint32_t(fieldsets_.size()));

  fieldsets_.insert(fieldsets_.end(), fieldset.begin(), fieldset.end());
  fieldsets_.push_back(crate::FieldIndex()); // terminator(~0)

  return fieldset_to_index_map[fieldset];
}

class Writer {
 public:
  Writer(const Scene &scene) : scene_(scene) {}

  const Scene &scene_;

  const std::string &Error() const { return err_; }

  bool WriteHeader() {
    char magic[8];
    magic[0] = 'P';
    magic[1] = 'X';
    magic[2] = 'R';
    magic[3] = '-';
    magic[4] = 'U';
    magic[5] = 'S';
    magic[6] = 'D';
    magic[7] = 'C';


    uint8_t version[8]; // Only first 3 bytes are used.
    version[0] = 0;
    version[1] = 8;
    version[2] = 0;

    // TOC offset(8bytes)
    // Must be 89 or greater.
    uint64_t toc_offset;

    std::array<uint8_t, 88> header;
    memset(&header, 0, 88);

    memcpy(&header[0], magic, 8);
    memcpy(&header[8], version, 8);
    memcpy(&header[16], &toc_offset, 8);

    oss_.write(reinterpret_cast<const char *>(&header[0]), 88);

    return true;
  }

  bool WriteTokens() {
    // Build single string rseparated by '\0', then compress it with lz4



    return false;
  }

  bool WriteTOC() {
    uint64_t num_sections = toc_.sections.size();

    DCOUT("# of sections = " << std::to_string(num_sections));

    if (num_sections == 0) {
      err_ += "Zero sections in TOC.\n";
      return false;
    }

    // # of sections
    oss_.write(reinterpret_cast<const char *>(&num_sections), 8);


    return true;
  }

  bool Write() {

    //
    //  - TOC
    //  - Tokens
    //  - Strings
    //  - Fields
    //  - FieldSets
    //  - Paths
    //  - Specs
    //

    if (!WriteTokens()) {
      PUSH_ERROR("Failed to write TOC.");
      return false;
    }

    if (!WriteTOC()) {
      PUSH_ERROR("Failed to write TOC.");
      return false;
    }

    // write heder
    oss_.seekp(0, std::ios::beg);
    if (!WriteHeader()) {
      PUSH_ERROR("Failed to write Header.");
      return false;
    }

    return true;
  }



  // Get serialized USDC binary data
  bool GetOutput(std::vector<uint8_t> *output) {
    if (!err_.empty()) {
      return false;
    }

    (void)output;

    // TODO
    return false;
  }

 private:
  Writer() = delete;
  Writer(const Writer &) = delete;

  TableOfContents toc_;

  //
  // Serialized data
  //
  std::ostringstream oss_;

  std::string err_;
};

}  // namespace

bool SaveAsUSDCToFile(const std::string &filename, const Scene &scene,
                std::string *warn, std::string *err) {
#ifdef __ANDROID__
  if (err) {
    (*err) += "Saving USDC to a file is not supported for Android platform.\n";
  }
  return false;
#else

  std::vector<uint8_t> output;

  if (!SaveAsUSDCToMemory(scene, &output, warn, err)) {
    return false;
  }

#ifdef _WIN32
#if defined(_MSC_VER) || defined(__GLIBCXX__)
  FILE *fp = nullptr;
  errno_t fperr = _wfopen_s(&fp, UTF8ToWchar(filename).c_str(), L"wb");
  if (fperr != 0) {
    if (err) {
      // TODO: WChar
      (*err) += "Failed to open file to write.\n";
    }
    return false;
  }
#else
  FILE *fp = nullptr;
  errno_t fperr = fopen_s(&fp, abs_filename.c_str(), "wb");
  if (fperr != 0) {
    if (err) {
      (*err) += "Failed to open file `" + filename + "` to write.\n";
    }
    return false;
  }
#endif

#else
  FILE *fp = fopen(filename.c_str(), "wb");
  if (fp == nullptr) {
    if (err) {
      (*err) += "Failed to open file `" + filename + "` to write.\n";
    }
    return false;
  }
#endif

  size_t n = fwrite(output.data(), /* size */1, /* count */output.size(), fp);
  if (n < output.size()) {
    // TODO: Retry writing data when n < output.size()

    if (err) {
      (*err) += "Failed to write data to a file.\n";
    }
    return false;
  }

  return true;
#endif
}

bool SaveAsUSDCToMemory(const Scene &scene, std::vector<uint8_t> *output,
                std::string *warn, std::string *err) {
  (void)warn;
  (void)output;

  // TODO
  Writer writer(scene);

  if (err) {
    (*err) += "USDC writer is not yet implemented.\n";
  }

  return false;
}

} // namespace usdc
}  // namespace tinyusdz