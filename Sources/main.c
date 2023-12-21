#include <stdio.h>
#include <3ds.h>
#include <3ds/svc.h>
#include <3ds/types.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <cwav.h>
#include <ncsnd.h>
#include <opusfile.h>

#include "math.h"
#include "sprite_animation_manager.h"

#include "vshader_shbin.h"

#include "road_t3x.h"
#include "stop_t3x.h"
#include "people_t3x.h"
#include "tunnel1_t3x.h"
#include "tunnel2_t3x.h"

#include "people_vertex.h"
#include "road_vertex.h"
#include "stop_vertex.h"
#include "tunnel.h"

#define CLEAR_COLOR 0xFFB100B9

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))


static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;
static C3D_Mtx projection;
static C3D_Mtx material =
{
	{
	{ { 0.0f, 0.2f, 0.2f, 0.2f } }, 
	{ { 0.0f, 0.4f, 0.4f, 0.4f } }, 
	{ { 0.0f, 0.8f, 0.8f, 0.8f } }, 
	{ { 1.0f, 0.5f, 0.5f, 0.5f } }, 
	}
};

static void* vbo_data;
static C3D_Tex road_tex, stop_tex, people_tex, tunnel_tex;
static C3D_FogLut fog_Lut;
static float angleY = 0.0, angleX = C3D_AngleFromDegrees(180), jumpZ = 20.0f;
static float jumpY = -50.0, angleW = 0.0, angleH = 0.0;
static float vector = 0.0;
static float verticalSpeed = 0.0f;
float initVy, gravity, vy, limitPy; 
int jumpFlag;

const u32 gasData[9] = {
	0xFF000000, 0xFF111111, 0xFF222222, 0xFF333333,
	0xFF444444, 0xFF555555, 0xFF666666, 0xFF777777,
	0xFF888888
};



static bool loadTextureFromMem(C3D_Tex* tex, C3D_TexCube* cube, const void* data, size_t size)
{
	Tex3DS_Texture t3x = Tex3DS_TextureImport(data, size, tex, cube, false);
	if (!t3x)
		return false;
	
	Tex3DS_TextureFree(t3x);
	return true;
}

static void sceneInit( void )
{
	// 提供されたバイナリデータから頂点シェーダーを解析し、初期化します
	vshader_dvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);

	// シェーダープログラムを初期化します
	shaderProgramInit(&program);

	// 解析された頂点シェーダーをシェーダープログラムに設定します
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);

	// レンダリングのためにシェーダープログラムをバインドします
	C3D_BindProgram(&program);

	// シェーダーのさまざまな変数に対するユニフォームの場所を取得します
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
	uLoc_lightVec   = shaderInstanceGetUniformLocation(program.vertexShader, "lightVec");
	uLoc_lightHalfVec = shaderInstanceGetUniformLocation(program.vertexShader, "lightHalfVec");
	uLoc_lightClr   = shaderInstanceGetUniformLocation(program.vertexShader, "lightClr");
	uLoc_material   = shaderInstanceGetUniformLocation(program.vertexShader, "material");

	// グラフィックサブシステムの属性情報を初期化します
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // 位置情報（3つの浮動小数点数）の属性ローダーを追加します
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // テクスチャ座標（2つの浮動小数点数）の属性ローダーを追加します
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // 法線（3つの浮動小数点数）の属性ローダーを追加します

	// レンダリングのための透視投影行列を設定します
	Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

	vbo_data = linearAlloc(sizeof(road_vertex_list) + sizeof(stop_vertex_list) + sizeof(people_vertex_list));
	memcpy(vbo_data, road_vertex_list, sizeof(road_vertex_list));
	memcpy((void*)((u32)vbo_data + sizeof(road_vertex_list)), stop_vertex_list, sizeof(stop_vertex_list));

	size_t road_size = sizeof(road_vertex_list);
	size_t stop_size = sizeof(stop_vertex_list);
	size_t people_size = sizeof(people_vertex_list);
	size_t tunnel_size = sizeof(tunnel_vertex_list);

	memcpy((void*)((u32)vbo_data + road_size + stop_size), people_vertex_list, people_size);
	memcpy((void*)((u32)vbo_data + road_size + stop_size + people_size), tunnel_vertex_list, tunnel_size);

	
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 3, 0x210);

	if(!loadTextureFromMem(&road_tex, NULL, road_t3x, road_t3x_size))
		svcBreak(USERBREAK_PANIC);
	C3D_TexSetFilter(&road_tex, GPU_LINEAR, GPU_NEAREST);

	if(!loadTextureFromMem(&stop_tex, NULL, stop_t3x, stop_t3x_size))
		svcBreak(USERBREAK_PANIC);
	C3D_TexSetFilter(&stop_tex, GPU_LINEAR, GPU_NEAREST);

	if(!loadTextureFromMem(&people_tex, NULL, people_t3x, people_t3x_size))
		svcBreak(USERBREAK_PANIC);
	C3D_TexSetFilter(&people_tex, GPU_LINEAR, GPU_NEAREST);

	if(!loadTextureFromMem(&tunnel_tex, NULL, tunnel1_t3x, tunnel1_t3x_size))
		svcBreak(USERBREAK_PANIC);
	C3D_TexSetFilter(&tunnel_tex, GPU_LINEAR, GPU_NEAREST);

 	C3D_GasLut gasLut;
    GasLut_FromArray(&gasLut, gasData);

    // ガスモードを設定（uy7d例として、GPU_FOG_NONEをフォグモードに、GPU_GAS_FOGにガスモードに設定）
    C3D_FogGasMode(GPU_FOG_NONE, GPU_GAS_FOG, false);

    // ガスルックアップテーブルをバインド
    C3D_GasLutBind(&gasLut);

}

static void sceneRender(void)
{
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
	Mtx_LookAt(&modelView, FVec3_New(angleW, jumpZ, jumpY), FVec3_New(angleW, jumpZ, angleH), FVec3_New(0, 1, 0), true);
	Mtx_Translate(&modelView, 0.0, 0.0, -2.0 + 0.5 * sinf(angleY), false);
	Mtx_RotateY(&modelView, angleX, false);
	Mtx_RotateX(&modelView, angleY, false);


	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_material, &material);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightVec, 0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightHalfVec, 0.0f, 0.0f, -1.0f, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightClr, 1.0f, 1.0f, 1.0f, 1.0f);

	C3D_TexBind(0, &road_tex);
	C3D_DrawArrays(GPU_TRIANGLES, 0, road_vertex_list_count);
	C3D_TexBind(0, &stop_tex);
	C3D_DrawArrays(GPU_TRIANGLES, road_vertex_list_count, stop_vertex_list_count);
	C3D_TexBind(0, &people_tex);
	C3D_DrawArrays(GPU_TRIANGLES, road_vertex_list_count + stop_vertex_list_count, people_vertex_list_count);
	C3D_TexBind(0, &tunnel_tex);
	C3D_DrawArrays(GPU_TRIANGLES, road_vertex_list_count + stop_vertex_list_count + people_vertex_list_count, tunnel_vertex_list_count);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);


}


static void sceneExit(void)
{
	C3D_TexDelete(&road_tex);
	C3D_TexDelete(&stop_tex);
	C3D_TexDelete(&people_tex);
	C3D_TexDelete(&tunnel_tex);

	linearFree(vbo_data);

	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}


static void drawText(const char* text)
{
    C2D_TextBuf textBuf = C2D_TextBufNew(4096);
	C2D_Font font = C2D_FontLoad("romfs:/onryou.bcfnt");
    C2D_Text textObj;
    C2D_TextFontParse(&textObj, font, textBuf, text);
    C2D_TextOptimize(&textObj);
    C2D_DrawText(&textObj, C2D_AlignCenter, 0, 8.0f, 8.0f, 0.5f, 0, 0);
}

int main(void)
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	consoleInit(GFX_BOTTOM, NULL);

		
	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	sceneInit();

	initVy = 20;
	gravity = -1.1f;
	jumpFlag = 0;
	vy = 1.5f;
	limitPy = jumpZ;

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		u32 kUp = hidKeysUp();
		if (kDown & KEY_START)
			break;

		if (kHeld & KEY_DOWN)
			angleY += C3D_AngleFromDegrees(2.0f);
		else if (kHeld & KEY_UP)
			angleY -= C3D_AngleFromDegrees(2.0f);
		else
		{
			if(angleY != 0)
			{
				if(angleY < C3D_AngleFromDegrees(0.0f))
				{
					angleY += C3D_AngleFromDegrees(3.0f);
					if(angleY > C3D_AngleFromDegrees(1.00f))
						angleY = C3D_AngleFromDegrees(0.0f);	
				}
				else
				{
					angleY -= C3D_AngleFromDegrees(3.0f);
					if(angleY < C3D_AngleFromDegrees(1.0f))
						angleY = C3D_AngleFromDegrees(0.0f);	
				}
			}
		}

		if (kHeld & KEY_RIGHT)
			angleX += C3D_AngleFromDegrees(2.0f);
		if (kHeld & KEY_LEFT)
			angleX -= C3D_AngleFromDegrees(2.0f);


		if (kHeld & KEY_B) {
			vector = 2.0f;
		}
		else if (kHeld & KEY_X) {
			
			vector = -2.0f;
		}

		if (jumpZ == 20 &&(kDown & KEY_A)) {
			vy = initVy;
			jumpFlag = 1;
		}

		if (jumpFlag == 1)
		{
			jumpZ +=  vy;
			vy += gravity;
			if (jumpZ < limitPy)
			{
				jumpZ = limitPy;
				jumpFlag = 0;
			}
		}
			
		if (kUp & KEY_B || kUp & KEY_X)
			vector = 0.0;

		if (angleY > C3D_AngleFromDegrees(50.0f)) {
			angleY = C3D_AngleFromDegrees(50.0f);
		} else if (angleY < C3D_AngleFromDegrees(-50.0f)) {
			angleY = C3D_AngleFromDegrees(-50.0f);
		}

		//-z
		if (jumpY >= 200) {
			jumpY = -60;
			
			//vector = 0.0;
		}
		//+z
		if (jumpY < -60) {
			// drawText("これ以上は進めないようだ");
			jumpY = -60;
			// angleX = C3D_AngleFromDegrees(180);
			//vector = 0.0;
		}

		//-X
		if (angleW < -30) {
			angleW = -30;
			vector = 0.0;
		}
		//+x
		if (angleW > 30) {
			angleW = 30;
			vector = 0.0;
		}
		
		jumpY += vector * cos(angleX);
		angleH += vector * cos(angleX);
		angleW -= vector * sin(angleX);
		angleH += verticalSpeed * cos(angleX); // Y軸の移動をcosによって角度に反映
    	jumpY -= verticalSpeed * sin(angleX); // Y軸の移動をsinによって角度に反映
		if (angleH < 0)
			angleH = -angleH;
		consoleClear();
		printf("X:%03f Y:%03f Z:%03f\n", angleW, jumpY, jumpZ);
		printf("angleX:%03f angleY:%03f angleH:%03f",angleX, angleY, angleH);
		
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
		C3D_FrameDrawOn(target);
		sceneRender();
		C3D_FrameEnd(0);
	}
	sceneExit();
	C3D_Fini();

	gfxExit();
	return 0;
}
