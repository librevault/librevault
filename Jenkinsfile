node {
    sh 'env > env.txt'
    readFile('env.txt').split("\r?\n").each {
        println it
    }
}
