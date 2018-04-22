#if (-NOT ($args.Count -eq 1))
#{
#    echo "Usage: IncrementVersion.ps1 AssemblyInfoFile"
#}
#else
#{
#	Add-Type -AssemblyName PresentationCore,PresentationFramework
#	$msgBoxInput =  [System.Windows.MessageBox]::Show('Would you like to increment version string?','IncreaseVersion','YesNo','Question')
#
#	if  ($msgBoxInput -eq 'Yes') 
#	{
#		$content = [IO.File]::ReadAllText($args[0])
#		$asmVerIdx = $content.IndexOf("AssemblyVersion(")
#		$asmVerIdx = $content.IndexOf("AssemblyVersion(", $asmVerIdx + 1)
#		$startIdx = $content.IndexOf(".", $asmVerIdx + 1)
#		$endIdx = $content.IndexOf(".", $startIdx + 1)
#		$minorVer = [int]$content.Substring($startIdx + 1, $endIdx - $startIdx)
#		$newContent = $content.Replace([string]$minorVer + ".*", [string]($minorVer + 1) + ".*")
#		[IO.File]::WriteAllText($args[0], $newContent, (New-Object System.Text.UTF8Encoding $true)) # write to UTF-8 with BOM
#		$startIdx = $newContent.IndexOf("`"", $asmVerIdx + 1) + 1
#		$endIdx = $newContent.IndexOf(".*", $startIdx + 1)
#		$version = $newContent.Substring($startIdx, $endIdx - $startIdx)
#		echo "Increase version to: $version"		
#	}
#}
$DEVENVFOLDER="$env:ProgramFiles (x86)"
if (!(Test-Path $DEVENVFOLDER)) { # not exist
	$DEVENVFOLDER="$env:ProgramFiles"
}

$DEVENV2017="$DEVENVFOLDER\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.com"

$PROJECT="SmartHiveViewer"

$FLASHDRIVE=gwmi win32_diskdrive | ?{$_.interfacetype -eq "USB"} | %{gwmi -Query "ASSOCIATORS OF {Win32_DiskDrive.DeviceID=`"$($_.DeviceID.replace('\','\\'))`"} WHERE AssocClass = Win32_DiskDriveToDiskPartition"} |  %{gwmi -Query "ASSOCIATORS OF {Win32_DiskPartition.DeviceID=`"$($_.DeviceID)`"} WHERE AssocClass = Win32_LogicalDiskToPartition"} | %{$_.deviceid}

$DEPLOYLOCATION="$FLASHDRIVE\SmartHive\Viewer"
if (!(Test-Path $DEPLOYLOCATION)) { # not exist
	echo ""
	echo "Error: Deploy errors occured."
	exit
}

echo ""
Invoke-Expression "& `"./IncreaseVersion.ps1`" ./$PROJECT/Properties/AssemblyInfo.cs"

echo ""
echo "Building $PROJECT..."
$process = Start-Process $DEVENV2017 "$PROJECT.sln /Rebuild Release" -PassThru -Wait -NoNewWindow
if ($process.ExitCode -ne 0) {
	echo ""
	echo "Error: Build errors occured."
	exit
}

echo ""
echo "Deploy $PROJECT..."
xcopy /v /Y /S $PROJECT\bin\Release $DEPLOYLOCATION
