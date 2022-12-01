// SPDX-License-Identifier: Apache 2.
// Copyright 2022 - Present, Light Transport Entertainment, Inc.
//
// Stage: Similar to Scene or Scene graph
#pragma once

#include "prim-types.hh"
#include "composition.hh"

namespace tinyusdz {

// TODO: Rename to LayerMetas
struct StageMetas {
  enum class PlaybackMode {
    PlaybackModeNone,
    PlaybackModeLoop,
  };
    
  // TODO: Support more predefined properties: reference = <pxrUSD>/pxr/usd/sdf/wrapLayer.cpp
  // Scene global setting
  TypedAttributeWithFallback<Axis> upAxis{Axis::Y}; // This can be changed by plugInfo.json in USD: https://graphics.pixar.com/usd/dev/api/group___usd_geom_up_axis__group.html#gaf16b05f297f696c58a086dacc1e288b5
  value::token defaultPrim;           // prim node name
  TypedAttributeWithFallback<double> metersPerUnit{1.0};        // default [m]
  TypedAttributeWithFallback<double> timeCodesPerSecond {24.0};  // default 24 fps
  TypedAttributeWithFallback<double> framesPerSecond {24.0};  // FIXME: default 24 fps
  TypedAttributeWithFallback<double> startTimeCode{0.0}; // FIXME: default = -inf?
  TypedAttributeWithFallback<double> endTimeCode{std::numeric_limits<double>::infinity()};
  std::vector<value::AssetPath> subLayers; // `subLayers`
  value::StringData comment; // 'comment' In Stage meta, comment must be string only(`comment = "..."` is not allowed)
  value::StringData doc; // `documentation`

  CustomDataType customLayerData; // customLayerData

  // USDZ extension
  TypedAttributeWithFallback<bool> autoPlay{true}; // default(or not authored) = auto play
  TypedAttributeWithFallback<PlaybackMode> playbackMode{PlaybackMode::PlaybackModeLoop};

  // Indirectly used.
  std::vector<value::token> primChildren;
};

class PrimRange;

// Similar to UsdStage, but much more something like a Scene(scene graph)
class Stage {
 public:

  // pxrUSD compat API ----------------------------------------
  static Stage CreateInMemory() {
    return Stage();
  }

  ///
  /// Traverse by depth-first order.
  /// NOTE: Not yet implementd. Use tydra::VisitPrims() for a while.
  ///
  // PrimRange Traverse();

  ///
  /// Get Prim at a Path.
  /// Path must be absolute Path.
  ///
  /// @returns pointer to Prim(to avoid a copy). Never return nullptr upon success.
  ///
  nonstd::expected<const Prim *, std::string> GetPrimAtPath(const Path &path) const;

  ///
  /// pxrUSD Compat API
  ///
  bool Flatten(bool addSourceFileComment = true) const {
    return compose(addSourceFileComment);
  }

  ///
  /// Dump Stage as ASCII(USDA) representation.
  ///
  std::string ExportToString() const;

  // pxrUSD compat API ----------------------------------------

  ///
  /// Get Prim from a children of given root Prim.
  /// Path must be relative Path.
  ///
  /// @returns pointer to Prim(to avoid a copy). Never return nullptr upon success.
  ///
  nonstd::expected<const Prim *, std::string> GetPrimFromRelativePath(const Prim &root, const Path &path) const;


  /// Find(Get) Prim at a Path.
  /// Path must be absolute Path.
  ///
  /// @param[in] path Absolute path(e.g. `/bora/dora`)
  /// @param[out] prim const reference to Prim(if found)
  /// @param[out] err Error message(filled when false is returned)
  ///
  /// @returns true if found a Prim.
  bool find_prim_at_path(const Path &path, const Prim *&prim, std::string *err = nullptr) const;

  /// Find(Get) Prim from a relative Path.
  /// Path must be relative Path.
  ///
  /// @param[in] root Find from this Prim
  /// @param[in] relative_path relative path(e.g. `dora/muda`)
  /// @param[out] prim const reference to Prim(if found)
  /// @param[out] err Error message(filled when false is returned)
  ///
  /// @returns true if found a Prim.
  bool find_prim_from_relative_path(const Prim &root, const Path &relative_path, const Prim *&prim, std::string *err) const;

  const std::vector<Prim> &root_prims() const {
    return root_nodes;
  }

  std::vector<Prim> &root_prims() {
    return root_nodes;
  }

  const StageMetas &metas() const {
    return stage_metas;
  }

  StageMetas &metas() {
    return stage_metas;
  }

  ///
  /// Assign unique Prim id inside this Stage.
  ///
  bool allocate_prim_id(uint64_t *prim_id) const;
  bool release_prim_id(const uint64_t prim_id) const;
  
  ///
  /// Call this function after you finished adding Prims manually to Stage.
  /// (No need to call this if you just use ether USDA/USDC/USDZ reader).
  ///
  /// - Compute absolute path and set it to Prim::abs_path for each Prim currently added to this Stage.
  /// - Assign unique ID to Prim(if Prim does not have prim_id)
  ///
  /// @return false when the Stage contains any invalid Prim 
  ///
  bool compute_absolute_prim_path_and_assign_prim_id();

  ///
  /// Compose scene(Not implemented yet).
  ///
  bool compose(bool addSourceFileComment = true) const;

 private:

  ///
  /// Loads USD from and return it as Layer
  ///
  bool LoadLayerFromFile(const std::string &filename, const LoadState load_state, Layer *layer);

  ///
  /// Loads USD asset from memory and return it as Layer
  bool LoadLayerFromMemory(const uint8_t *addr, const size_t nbytes, const std::string &asset_name, const LoadState load_state, Layer *dest);

  ///
  /// Loads `reference` USD asset and return it as Layer
  ///
  bool LoadReference(const Reference &reference, Layer *dest);

  ///
  /// Loads USD assets described in `subLayers` Stage meta and return it as Layers
  ///
  bool LoadSubLayers(std::vector<Layer> *dest_sublayers);

  // Root nodes
  std::vector<Prim> root_nodes;

  std::string name;       // Scene name
  int64_t default_root_node{-1};  // index to default root node

  StageMetas stage_metas;

  mutable std::string _err;
  mutable std::string _warn;

  // Cached prim path.
  // key : prim_part string (e.g. "/path/bora")
  mutable std::map<std::string, const Prim *> _prim_path_cache;

  mutable bool _dirty{false}; // True when Stage content changes(addition, deletion, composition/flatten, etc.)

  mutable HandleAllocator<uint64_t> _prim_id_allocator;

};

} // namespace tinyusdz
