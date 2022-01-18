import java.text.SimpleDateFormat
import groovy.json.JsonSlurperClassic


node 
{
		withEnv([
			"UE4DIST_PATH=${params.UE4DIST_PATH?params.UE4DIST_PATH:env.UE4DIST_PATH}"
		])
		{
			withEnv([
				"GIT_URL=https://github.com/gee03143/BuildServerTest.git",
				"GIT_BRANCH=${env.GIT_BRANCH?env.GIT_BRANCH:'master'}",
				"PLATFORM=${env.PLATFORM?env.PLATFORM:'Win64'}",	
				"BUILD_TYPE=${env.BUILD_TYPE?env.BUILD_TYPE:'FullGame'}",	// 'FullGame' or 'MinimalGame'
				"BUILDPATCHTOOL_PATH=${UE4DIST_PATH}\\Engine\\Binaries\\Win64\\BuildPatchTool.exe"
			])
			{
				stage('Preparation')
				{
					dir('git')
					{
					git branch: "main", url: "${GIT_URL}"
					gitCommitHash = sh(returnStdout: true, script: 'git rev-parse HEAD').trim()
					buildName = readFile(file:"ShootingGame/Source/BuildName.txt").replace("\"", "").trim()
					gameVersion = readFile(file:"ShootingGame/Source/GameVersion.txt").replace("\"", "").trim()
					}
				}
				if(BUILD_TYPE == 'FullGame' || (BUILD_TYPE == 'MinimalGame' && env.CreateReleaseVersion == 'true'))
				{
					stage('Build')
					{
						withEnv([
						"COOK_FLAVOR=${env.COOK_FLAVOR?env.COOK_FLAVOR:'ASTC'}",
						"RELEASE_VERSION=${gameVersion}"
						])
						{
							bat "rmdir /s /q ${WORKSPACE}\\git\\ShootingGame\\Binaries\\${env.PLATFORM} || true"

							def buildCommandLine = "call ${UE4DIST_PATH}\\Engine\\Build\\BatchFiles\\RunUAT.bat BuildCookRun -project=\"${WORKSPACE}\\git\\ShootingGame\\ShootingGame.uproject\" -build -noP4 -platform=${PLATFORM} -targetplatform=${PLATFORM} -cookflavor=${COOK_FLAVOR} -cook -stage -package -compressed -pak -utf8output"

							def defaultGamePath = "${WORKSPACE}\\git\\ShootingGame\\Config\\DefaultGame.ini"
							def defaultEnginePath = "${WORKSPACE}\\git\\ShootingGame\\Config\\DefaultEngine.ini"

							if(BUILD_TYPE == 'MinimalGame')
							{
								buildCommandLine+= " -manifests"
							}

							bat buildCommandLine
						}
					}
				}		
				if(BUILD_TYPE == 'MinimalGame')
				{
					stage('Generate Patch')
					{
						withEnv([
							"COOK_FLAVOR=${env.COOK_FLAVOR?env.COOK_FLAVOR:'ASTC'}",
							"DATA_VERSION=${env.DATA_VERSION}",
							"RELEASE_VERSION=${gameVersion}"
						])
						{
							def buildCommandLine = "call ${UE4DIST_PATH}\\Engine\\Build\\BatchFiles\\RunUAT.bat BuildCookRun -project=\"${WORKSPACE}\\git\\ShootingGame\\ShootingGame.uproject\" -build -noP4  -platform=${PLATFORM} -targetplatform=${PLATFORM} -cookflavor=${COOK_FLAVOR} -cook -stage -compressed -pak -utf8output"
							buildCommandLine += " -manifests -generatepatch -BasedOnReleaseVersion=${RELEASE_VERSION}"

							bat buildCommandLine

							def pakDir = "${WORKSPACE}\\git\\ShootingGame\\Saved\\StagedBuilds"
							if (PLATFORM == 'Win64')
							{
							pakDir += "\\WindowsNoEditor\\ShootingGame\\Content\\Paks"
							}
							else if (PLATFORM == 'IOS')
							{
								pakDir += "\\IOS\\cookeddata\\ShootingGame\\content\\paks"
							}
							else if (PLATFORM == 'Android')
							{
								pakDir += "\\Android_${COOK_FLAVOR}\\ShootingGame\\Content\\Paks"
							}
							pakDir = pakDir.replace("\\", "/")
							def fileNames = sh(returnStdout: true, script: "ls -1 ${pakDir}").split()
							for (fileName in fileNames)
							{
								if (!fileName.endsWith("_P.pak") && !fileName.endsWith("_p.pak"))
								{
									bat "rm ${pakDir}/${fileName} || true"
								}
							}

							def cloudDir = "${env.WWW_ROOT}\\PatchDist\\Shootergame\\${env.PLATFORM}"
							bat "$BUILDPATCHTOOL_PATH -mode=PatchGeneration -BuildRoot=\"$pakDir\" -CloudDir=$cloudDir -AppName=${buildName} -BuildVersion=${DATA_VERSION} -AppLaunch=\"\" -AppArgs=\"\""

							def manifest = "${buildName}${DATA_VERSION}.manifest"
							echo "${manifest}"

							dir (cloudDir)
								{
									sh "cp ${manifest} test.manifest"

									if (env.PUBLIC_WEBDIST_ROOT?.trim())
									{
										String dir ="${cloudDir}"
										dir = dir.replace("\\", "/");
										dir = dir.replace(":", "");
										dir = "/cygdrive/" + dir
										bat "${RSYNC_PATH}  -v -rlt -z --chmod=a=rw,Da+x --delete --relative \"/cygdrive/E/wwwroot/./PatchDist/${buildName}\" \"${env.PUBLIC_WEBDIST_ROOT}/\""
									}
								}
						}
					}
				}
		
				if(env.UPLOAD_CHUNK == 'true')
				{
					withEnv([
						"COOK_FLAVOR=${env.COOK_FLAVOR?env.COOK_FLAVOR:'ASTC'}",
						"ARCHIVE_DIRECTORY=${env.ARCHIVE_DIRECTORY?env.ARCHIVE_DIRECTORY:env.WWW_ROOT + "\\${BUILD_TYPE}\\" + buildName}",
						"ARCHIVE_NAME=TEST_${gameVersion}"
					])
					{
						def buildRoot = "${ARCHIVE_DIRECTORY}\\${ARCHIVE_NAME}"
						if (env.PLATFORM == 'Windows')
						{
							buildRoot += "\\WindowsNoEditor"
						}
						else if (env.PLATFORM == 'Android')
						{
							buildRoot += "\\Android_${COOK_FLAVOR}"
						}
						else if (env.PLATFORM == 'IOS')
						{
							buildRoot += "\\IOS"
						}

						def cloudDir = "${env.WWW_ROOT}\\${BUILD_TYPE}Dist\\${buildName}\\${env.PLATFORM}"
						def manifest = "${buildName}-${env.BUILD_NUMBER}.manifest"

						stage('Make Chunk')
						{
							bat "$BUILDPATCHTOOL_PATH -mode=PatchGeneration -BuildRoot=\"$buildRoot\" -CloudDir=$cloudDir -AppName=${buildName} -BuildVersion=-${env.BUILD_NUMBER} -AppLaunch=\"\" -AppArgs=\"\""

							echo "${manifest}"
						}
						stage('Upload Chunk')
						{
							dir (cloudDir)
							{
								sh "cp ${manifest} ${buildName}-release.manifest"

								if (env.PUBLIC_WEBDIST_ROOT?.trim())
								{
									bat "${RSYNC_PATH}  -v -rlt -z --chmod=a=rw,Da+x --delete --relative \"/cygdrive/E/wwwroot/./${BUILD_TYPE}Dist/${buildName}/${env.PLATFORM}/ChunksV3\" \"${env.PUBLIC_WEBDIST_ROOT}/\""
									bat "${RSYNC_PATH}  -v -rlt -z --chmod=a=rw,Da+x --delete --relative --exclude='ChunksV3/' \"/cygdrive/E/wwwroot/./${BUILD_TYPE}Dist/${buildName}/${env.PLATFORM}\" \"${env.PUBLIC_WEBDIST_ROOT}/\""
								}
							}
						}
					}
				}
			}	//inner withenv
		}	//outer withenv
}

