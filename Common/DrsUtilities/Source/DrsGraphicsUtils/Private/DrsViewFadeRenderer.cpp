// Copyright (c) 2026 Steven Wright. Licensed under the MIT License.

#include "DrsViewFadeRenderer.h"
#include "ClearQuad.h"
#include "DrsViewFadeLibrary.h"
#include "GlobalShader.h"
#include "OneColorShader.h"
#include "PipelineStateCache.h"
#include "RenderGraphUtils.h"
#include "RenderUtils.h"
#include "RHIStaticStates.h"
#include "SceneView.h"

namespace
{
	// Holdout: color = dst * (1 - a), alpha = a + dst * (1 - a). Matches the AlphaHoldout material blend mode.
	using THoldoutBlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_Zero, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>;

	// Fade: color = src * a + dst * (1 - a), alpha = dst * (1 - a).
	using TFadeBlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_InverseSourceAlpha>;

	void DrawFullScreenQuad(FRHICommandList& rhiCmdList, FRHIBlendState* blendState, const FLinearColor& color)
	{
		FGraphicsPipelineStateInitializer graphicsPsoInit;
		rhiCmdList.ApplyCachedRenderTargets(graphicsPsoInit);
		graphicsPsoInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		graphicsPsoInit.BlendState = blendState;
		graphicsPsoInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

		FGlobalShaderMap* shaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<TOneColorVS<true>> vertexShader(shaderMap);

		TOneColorPixelShaderMRT::FPermutationDomain permutationVector;
		permutationVector.Set<TOneColorPixelShaderMRT::TOneColorPixelShaderNumOutputs>(1);
		permutationVector.Set<TOneColorPixelShaderMRT::TOneColorPixelShader128bitRT>(PlatformRequires128bitRT(static_cast<EPixelFormat>(graphicsPsoInit.RenderTargetFormats[0])));
		TShaderMapRef<TOneColorPixelShaderMRT> pixelShader(shaderMap, permutationVector);

		graphicsPsoInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		graphicsPsoInit.BoundShaderState.VertexShaderRHI = vertexShader.GetVertexShader();
		graphicsPsoInit.BoundShaderState.PixelShaderRHI = pixelShader.GetPixelShader();
		graphicsPsoInit.PrimitiveType = PT_TriangleStrip;

		SetGraphicsPipelineState(rhiCmdList, graphicsPsoInit, 0);
		rhiCmdList.SetStencilRef(1);

		FRHIBatchedShaderParameters& batchedParameters = rhiCmdList.GetScratchShaderParameters();
		vertexShader->SetParameters(batchedParameters, 0.0f);
		rhiCmdList.SetBatchedShaderParameters(rhiCmdList.GetBoundVertexShader(), batchedParameters);

		TOneColorPixelShaderMRT::FParameters parameters;
		pixelShader->FillParameters(parameters, &color, 1);
		SetShaderParameters(rhiCmdList, pixelShader, pixelShader.GetPixelShader(), parameters);

		rhiCmdList.SetStreamSource(0, GClearVertexBuffer.VertexBufferRHI, 0);
		rhiCmdList.DrawPrimitive(0, 2, 1);
	}
}

void FDrsViewFadeRenderer::PostRenderView_RenderThread(FRDGBuilder& graphBuilder, FSceneView& inView)
{
	AddPass(graphBuilder, RDG_EVENT_NAME("DrsViewFade"), [this, &inView](FRHICommandListImmediate& rhiCmdList)
	{
		FLinearColor fadeColor = FadeColor;
		fadeColor.A = FadeOpacity;
		const float holdoutOpacity = HoldoutOpacity;

		const FTextureRHIRef renderTarget = inView.Family->RenderTarget->GetRenderTargetTexture();
		if (!renderTarget.IsValid())
		{
			return;
		}

		FRHIRenderPassInfo renderPassInfo(renderTarget, ERenderTargetActions::Load_Store);
		if (inView.bIsMobileMultiViewEnabled)
		{
			renderPassInfo.MultiViewCount = 2;
		}

		const FIntRect& viewport = inView.UnscaledViewRect;
		rhiCmdList.BeginRenderPass(renderPassInfo, TEXT("DrsViewFade"));
		rhiCmdList.SetViewport(static_cast<float>(viewport.Min.X), static_cast<float>(viewport.Min.Y), 0.0f, static_cast<float>(viewport.Max.X), static_cast<float>(viewport.Max.Y), 1.0f);

		// The holdout's colour is irrelevant; only its alpha is used.
		if (!FMath::IsNearlyZero(holdoutOpacity))
		{
			DrawFullScreenQuad(rhiCmdList, THoldoutBlendState::GetRHI(), FLinearColor(0.0f, 0.0f, 0.0f, holdoutOpacity));
		}
		if (!FMath::IsNearlyZero(fadeColor.A))
		{
			DrawFullScreenQuad(rhiCmdList, TFadeBlendState::GetRHI(), fadeColor);
		}

		rhiCmdList.EndRenderPass();
	});
}

bool FDrsViewFadeRenderer::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& context) const
{
	return !FMath::IsNearlyZero(FadeOpacity) || !FMath::IsNearlyZero(HoldoutOpacity);
}

void FDrsViewFadeRenderer::SetColor(const FLinearColor& color, bool shouldIgnoreAlpha)
{
	FadeColor = color;
	if (!shouldIgnoreAlpha)
	{
		SetOpacity(color.A);
	}
}

void FDrsViewFadeRenderer::SetOpacity(float opacity)
{
	const bool wasOpaque = IsOpaque();
	FadeOpacity = opacity;
	if (IsOpaque() != wasOpaque)
	{
		UDrsViewFadeLibrary::OnViewFadeOpaqueChanged.Broadcast(!wasOpaque);
	}
}
