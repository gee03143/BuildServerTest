import java.text.SimpleDateFormat
import groovy.json.JsonSlurperClassic


node 
{
	withEnv([
		"UE4DIST_PATH=${params.UE4DIST_PATH?params.UE4DIST_PATH:env.UE4DIST_PATH}"
	])
	stage('Preparation')
	{
		dir('git)
		{
			git branch: "master", url: "https://github.com/gee03143/BuildServerTest.git"
		}
	}
	stage('Build')
	{
		def buildCommandLine = "call ${UE4DIST_PATH}\\Engine\\Build\\BatchFiles\\RunUAT.bat BuildCookRun -project=\"${WORKSPACE}\\git\\BuildServerTest\ShootingGame\ShootingGame.uproject\" -clientconfig=Development -nodebuginfo -targetplatform=Win64 -cook -stage -pak
	}
}

