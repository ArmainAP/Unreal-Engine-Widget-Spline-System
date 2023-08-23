// Copyright to Kat Code Labs, SRL. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class FSplineWidgetDetailCustomization final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
};
