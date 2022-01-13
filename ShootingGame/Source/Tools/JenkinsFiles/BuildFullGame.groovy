import java.text.SimpleDateFormat
import groovy.json.JsonSlurperClassic

node 
{
    withEnv([
		"PLATFORM=${env.PLATFORM?env.PLATFORM:'Android'}",
	])
	{
    	stage('Stage 1')
    	{
    		echo 'hello world!'
    	}
    	stage('Stage 2')
    	{
    		echo 'Stage 2'
    	}
    	stage('Stage 3')
    	{
    	    echo 'Stage 3'
	    }
	    stage('echo platform')
	    {
	        echo PLATFORM
	    }
	}
}
