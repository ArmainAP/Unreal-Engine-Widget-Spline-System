// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

struct WIDGETSPLINESYSTEM_API FSlatePaintContext
{
	FSlatePaintContext() = delete;
	FSlatePaintContext(FSlateWindowElementList& OutDrawElements, const FGeometry& InGeometry, const int32 InLayerId, const ESlateDrawEffect InDrawEffect, const FColor InTintColor)
		: OutDrawElements(OutDrawElements)
		, AllotedGeometry(InGeometry)
		, DrawEffect(InDrawEffect)
		, TintColor(InTintColor)
		, LayerId(InLayerId)
	{
		PaintGeometry = AllotedGeometry.ToPaintGeometry();
	}

	const FSlateRenderTransform& GetRenderTransform() const
	{
		PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
		return PaintGeometry.GetAccumulatedRenderTransform();
	}

	FSlateWindowElementList& OutDrawElements;
	const FGeometry& AllotedGeometry;
	const ESlateDrawEffect DrawEffect;
	const FColor TintColor;
	FPaintGeometry PaintGeometry;
	int32 LayerId;
};