#include <algorithm>
#include <iostream>
#include <sstream>

#include "tinyusdz.hh"
#include "tydra/render-data.hh"
#include "usdShade.hh"
#include "pprinter.hh"
#include "prim-pprint.hh"
#include "value-pprint.hh"

static std::string GetFileExtension(const std::string &filename) {
  if (filename.find_last_of('.') != std::string::npos)
    return filename.substr(filename.find_last_of('.') + 1);
  return "";
}

static std::string str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 // static_cast<int(*)(int)>(std::tolower)         // wrong
                 // [](int c){ return std::tolower(c); }           // wrong
                 // [](char c){ return std::tolower(c); }          // wrong
                 [](unsigned char c) { return std::tolower(c); }  // correct
  );
  return s;
}

// key = Full absolute prim path(e.g. `/bora/dora`)
using MaterialMap = std::map<std::string, const tinyusdz::Material *>;
using PreviewSurfaceMap =
    std::map<std::string, const tinyusdz::UsdPreviewSurface *>;
using UVTextureMap = std::map<std::string, const tinyusdz::UsdUVTexture *>;
using PrimvarReader_float2Map =
    std::map<std::string, const tinyusdz::UsdPrimvarReader_float2 *>;

template <typename T>
static bool TraverseRec(const std::string &path_prefix,
                        const tinyusdz::Prim &prim, uint32_t depth,
                        std::map<std::string, const T *> &itemmap) {
  if (depth > 1024 * 128) {
    // Too deep
    return false;
  }

  std::string prim_abs_path = path_prefix + "/" + prim.local_path().full_path_name();

  if (prim.is<tinyusdz::Material>()) {
    if (const T *pv = prim.as<T>()) {
      std::cout << "Path : <" << prim_abs_path << "> is "
                << tinyusdz::value::TypeTraits<T>::type_name() << ".\n";
      itemmap[prim_abs_path] = pv;
    }
  }

  for (const auto &child : prim.children()) {
    if (!TraverseRec(prim_abs_path, child, depth + 1, itemmap)) {
      return false;
    }
  }

  return true;
}

template <typename T>
static bool TraverseShaderRec(const std::string &path_prefix,
                        const tinyusdz::Prim &prim, uint32_t depth,
                        std::map<std::string, const T *> &itemmap) {
  if (depth > 1024 * 128) {
    // Too deep
    return false;
  }

  std::string prim_abs_path = path_prefix + "/" + prim.local_path().full_path_name();

  // First test if Shader prim.
  if (const tinyusdz::Shader *ps = prim.as<tinyusdz::Shader>()) {
    // Concrete Shader object(e.g. UsdUVTexture) is stored in .data.
    if (const T *s = ps->value.as<T>()) {
      std::cout << "Path : <" << prim_abs_path << "> is "
                << tinyusdz::value::TypeTraits<T>::type_name() << ".\n";
      itemmap[prim_abs_path] = s;
    }
  }

  for (const auto &child : prim.children()) {
    if (!TraverseShaderRec(prim_abs_path, child, depth + 1, itemmap)) {
      return false;
    }
  }

  return true;
}

static void TraverseMaterial(const tinyusdz::Stage &stage, MaterialMap &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseRec(/* root */ "", prim, 0, m);
  }
}

static void TraversePreviewSurface(const tinyusdz::Stage &stage,
                                   PreviewSurfaceMap &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseShaderRec(/* root */ "", prim, 0, m);
  }
}

static void TraverseUVTexture(const tinyusdz::Stage &stage, UVTextureMap &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseShaderRec(/* root */ "", prim, 0, m);
  }
}

static void TraversePrimvarReader_float2(const tinyusdz::Stage &stage,
                                         PrimvarReader_float2Map &m) {
  for (const auto &prim : stage.GetRootPrims()) {
    TraverseShaderRec(/* root */ "", prim, 0, m);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Need input.usdz\n" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filepath = argv[1];
  std::string warn;
  std::string err;

  std::string ext = str_tolower(GetFileExtension(filepath));

  tinyusdz::Stage stage;

  if (ext.compare("usdc") == 0) {
    bool ret = tinyusdz::LoadUSDCFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USDC file: " << filepath << "\n";
      return EXIT_FAILURE;
    }
  } else if (ext.compare("usda") == 0) {
    bool ret = tinyusdz::LoadUSDAFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USDA file: " << filepath << "\n";
      return EXIT_FAILURE;
    }
  } else if (ext.compare("usdz") == 0) {
    // std::cout << "usdz\n";
    bool ret = tinyusdz::LoadUSDZFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USDZ file: " << filepath << "\n";
      return EXIT_FAILURE;
    }

  } else {
    // try to auto detect format.
    bool ret = tinyusdz::LoadUSDFromFile(filepath, &stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
      // return EXIT_FAILURE;
    }

    if (!ret) {
      std::cerr << "Failed to load USD file: " << filepath << "\n";
      return EXIT_FAILURE;
    }
  }

  std::string s = stage.ExportToString();
  std::cout << s << "\n";
  std::cout << "--------------------------------------"
            << "\n";

  // Mapping hold the pointer to concrete Prim object,
  // So stage content should not be changed(no Prim addition/deletion).
  MaterialMap matmap;
  PreviewSurfaceMap surfacemap;
  UVTextureMap texmap;
  PrimvarReader_float2Map preadermap;

  TraverseMaterial(stage, matmap);
  TraversePreviewSurface(stage, surfacemap);
  TraverseUVTexture(stage, texmap);
  TraversePrimvarReader_float2(stage, preadermap);

  // Query example
  for (const auto &item : matmap) {
    nonstd::expected<const tinyusdz::Prim*, std::string> mat = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (mat) {
      std::cout << "Found Material <" << item.first << "> from Stage:\n";
      if (const tinyusdz::Material *mp = mat.value()->as<tinyusdz::Material>()) { // this should be true though.
        std::cout << tinyusdz::to_string(*mp) << "\n";
      }
    } else {
      std::cerr << "Err: " << mat.error() << "\n";
    }
  }

  for (const auto &item : surfacemap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Found Shader(UsdPreviewSurface) <" << item.first << "> from Stage:\n";
    
      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.
        std::cout << tinyusdz::to_string(*sp) << "\n";

        if (const tinyusdz::UsdPreviewSurface *surf = sp->value.as<tinyusdz::UsdPreviewSurface>()) {
          // TODO: ppriter for UsdPreviewSurface
          (void)surf;
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }

  for (const auto &item : texmap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Found Shader(UsdUVTexture) <" << item.first << "> from Stage:\n";
    
      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.
        std::cout << tinyusdz::to_string(*sp) << "\n";

        if (const tinyusdz::UsdUVTexture *tex = sp->value.as<tinyusdz::UsdUVTexture>()) {
          // TODO: ppriter for UsdUVTexture
          (void)tex;
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }

  for (const auto &item : preadermap) {
    // Returned Prim is Shader class
    nonstd::expected<const tinyusdz::Prim*, std::string> shader = stage.GetPrimAtPath(tinyusdz::Path(item.first, /* prop name */""));
    if (shader) {
      std::cout << "Found Shader(UsdPrimvarReader_float2) <" << item.first << "> from Stage:\n";
    
      const tinyusdz::Shader *sp = shader.value()->as<tinyusdz::Shader>();
      if (sp) { // this should be true though.
        std::cout << tinyusdz::to_string(*sp) << "\n";

        if (const tinyusdz::UsdPrimvarReader_float2 *tex = sp->value.as<tinyusdz::UsdPrimvarReader_float2>()) {
          // TODO: ppriter for UsdUVTexture
          (void)tex;
        }
      }

    } else {
      std::cerr << "Err: " << shader.error() << "\n";
    }
  }

  return 0;
}
