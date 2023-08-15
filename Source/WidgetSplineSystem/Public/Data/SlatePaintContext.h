// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

struct WIDGETSPLINESYSTEM_API FSlatePaintContext
{
	FSlatePaintContext() = delete;
	FSlatePaintContext(FSlateWindowElementList& OutDrawElements, const FPaintGeometry& InPaintGeometry, const int32 InLayerId, const ESlateDrawEffect InDrawEffect, const FColor InTintColor)
		: OutDrawElements(OutDrawElements)
		, PaintGeometry(InPaintGeometry)
		, DrawEffect(InDrawEffect)
		, TintColor(InTintColor)
		, LayerId(InLayerId)
	{}

	const FSlateRenderTransform& GetRenderTransform() const
	{
		PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
		return PaintGeometry.GetAccumulatedRenderTransform();
	}

	FSlateWindowElementList& OutDrawElements;
	const FPaintGeometry& PaintGeometry;
	const ESlateDrawEffect DrawEffect;
	const FColor TintColor;
	const int32 LayerId;
};