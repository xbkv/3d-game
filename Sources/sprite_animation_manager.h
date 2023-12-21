/**
 * @file sprite_animation_manager.h
 * @brief 画面上の2Dオブジェクトの初期化と描画
 */
#ifndef SPRITE_ANIMATION_MANAGER_H
#define SPRITE_ANIMATION_MANAGER_H

#include <citro2d.h>
#include <3ds/svc.h>
#include <3ds/types.h>

#include <stdbool.h>

#define ANIMATION_REFRESH_TIME_MIN  17   // アニメーションのリフレッシュ時間の最小値は17ms

typedef struct sprite_velocity {
    float dx; ///< x方向のスプライト速度
    float dy; ///< y方向のスプライト速度
} sprite_velocity_t;

typedef struct sprite_pivot {
    float x; ///< スプライトのx軸ピボットポイント
    float y; ///< スプライトのy軸ピボットポイント
} sprite_pivot_t;

typedef struct sprite_position {
    float x; ///< デカルト座標系でのスプライトのx位置
    float y; ///< デカルト座標系でのスプライトのy位置
} sprite_position_t;

typedef struct sprite_refresh_info {
    uint64_t start; ///< 開始時間
    uint64_t stop; ///< 終了時間
    uint64_t elapsed; ///< 経過時間（`start` - `stop`）
    uint64_t refresh_time; ///< 次のスプライト更新時間 [単位: ms]
} sprite_refresh_info_t;

typedef struct sprite_frame_info {
    int current_frame_index; ///< 現在のスプライトID番号
    size_t num_of_sprites; ///< スプライトの数
    bool loop_once; ///< スプライトアニメーションのループ情報
} sprite_frame_info_t;

typedef struct object_2d_info {
    C2D_Sprite* object_sprite; ///< 2Dオブジェクトスプライト情報
    C2D_SpriteSheet spritesheet; ///< 2Dオブジェクトのスプライトシート情報
    sprite_position_t position; ///< 2Dオブジェクトの位置情報
    sprite_velocity_t position_velocity; ///< 2Dオブジェクトの位置速度情報
    sprite_pivot_t pivot; ///< 2Dオブジェクトのピボット情報
    float rotation; ///< 2Dオブジェクトの度数単位の回転情報
    float rotation_velocity; ///< 2Dオブジェクトの回転速度情報
    sprite_refresh_info_t refresh_info; ///< 2Dオブジェクトのリフレッシュ情報
    sprite_frame_info_t frame_info; ///< 2Dオブジェクトのフレーム情報
} object_2d_info_t;

/**
 * @defgroup Initialization 初期化関数群
 * @{
 */

/**
 * @brief 2Dオブジェクトを初期化します。
 * 
 * @param[in] object 2Dオブジェクト
 * @param[in] sprites スプライトシートから取得するスプライトの配列
 * @param[in] filename スプライトシート情報を含むファイルのパス
 * @param[in] pivot 初期のピボットポイント
 * @param[in] position 初期位置
 * @param[in] rotation 初期回転
 * @param[in] animation_refresh_time スプライトアニメーションのリフレッシュ時間 [単位: ms]
 * @param[in] loop_once スプライトアニメーションを一度再生するか、永遠に再生するかの情報
 * @returns None
 */
void initialize_object(object_2d_info_t* object, C2D_Sprite* sprites, const char* filename, const sprite_pivot_t pivot, const sprite_position_t position, const float rotation, uint64_t animation_refresh_time, bool loop_once);

/**
 * @brief 2Dオブジェクトを非初期化します。
 * 
 * @param[in] object 2Dオブジェクト
 * @returns None
 */
void deinitialize_object(object_2d_info_t* object);

/**
 * 初期化関数群の終わり
 * @}
 */

/**
 * @defgroup Update 更新関数群
 * @{ 
 */

/**
 * @brief オブジェクトのポーズを更新します。
 * 
 * @param[in] object 2Dオブジェクト
 * @returns None
 */
void update_object(object_2d_info_t* object);

/**
 * @brief 2Dオブジェクトからスプライトアニメーションを描画します。
 * 
 * @param[in] object 2Dオブジェクト
 * @returns None
 */
void draw_sprite_animation(object_2d_info_t* object);

/**
 * 更新関数群の終わり
 * @}
 */

#endif /* SPRITE_ANIMATION_MANAGER_H */