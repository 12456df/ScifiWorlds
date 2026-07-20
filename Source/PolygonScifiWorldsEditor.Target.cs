// Copyright (c) 2024 Synty Studios Limited. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PolygonScifiWorldsEditorTarget : TargetRules
{
	public PolygonScifiWorldsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "PolygonScifiWorlds" } );
	}
}
