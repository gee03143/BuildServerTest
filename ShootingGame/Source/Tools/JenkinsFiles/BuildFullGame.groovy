import java.text.SimpleDateFormat
import groovy.json.JsonSlurperClassic


node 
{
	stage('Stage 1')
	{
		echo 'hello world!'
	}
	stage('Stage 2')
	{
		echo 'Stage 2'
	}
}