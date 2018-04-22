if (-NOT ($args.Count -eq 1))
{
    echo "Usage: IncrementVersion.ps1 AssemblyInfoFile"
}
else
{
	Add-Type -AssemblyName PresentationCore,PresentationFramework
	$msgBoxInput =  [System.Windows.MessageBox]::Show('Would you like to increment version string?','IncreaseVersion','YesNo','Question')

	if  ($msgBoxInput -eq 'Yes') 
	{
		$content = [IO.File]::ReadAllText($args[0])
		$asmVerIdx = $content.IndexOf("AssemblyVersion(")
		$asmVerIdx = $content.IndexOf("AssemblyVersion(", $asmVerIdx + 1)
		$startIdx = $content.IndexOf(".", $asmVerIdx + 1)
		$endIdx = $content.IndexOf(".", $startIdx + 1)
		$minorVer = [int]$content.Substring($startIdx + 1, $endIdx - $startIdx)
		$newContent = $content.Replace([string]$minorVer + ".*", [string]($minorVer + 1) + ".*")
		[IO.File]::WriteAllText($args[0], $newContent, (New-Object System.Text.UTF8Encoding $true)) # write to UTF-8 with BOM
		$startIdx = $newContent.IndexOf("`"", $asmVerIdx + 1) + 1
		$endIdx = $newContent.IndexOf(".*", $startIdx + 1)
		$version = $newContent.Substring($startIdx, $endIdx - $startIdx)
		echo "Increase version to: $version"		
	}
}