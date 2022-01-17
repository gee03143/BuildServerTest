import java.text.SimpleDateFormat
import groovy.json.JsonSlurperClassic


node 
{
	withEnv([
		"UE4DIST_PATH=${params.UE4DIST_PATH?params.UE4DIST_PATH:env.UE4DIST_PATH}"
	])
	{
		"GIT_URL=https://github.com/gee03143/BuildServerTest.git"
	}
	stage('Preparation')
	{
		dir('git')
		{
			git branch: "main", url: "https://github.com/gee03143/BuildServerTest.git"
			gitCommitHash = sh(returnStdout: true, script: 'git rev-parse HEAD').trim()
		}
	}
	stage('Build')
	{
		def buildCommandLine = "call ${UE4DIST_PATH}\\Engine\\Build\\BatchFiles\\RunUAT.bat BuildCookRun -project=\"${WORKSPACE}\\git\\ShootingGame\\ShootingGame.uproject\" -build -noP4 -clientconfig=Development -nodebuginfo -targetplatform=Win64 -cook -stage -pak"
		bat buildCommandLine
	}
}

