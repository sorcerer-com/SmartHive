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
