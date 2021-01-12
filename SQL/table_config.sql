CREATE TABLE sensor1
( 
    uid INT(11) NOT NULL AUTO_INCREMENT,  
    timestamp DATETIME NOT NULL,  
    co2_concentration INT(5) NOT NULL,  
    PRIMARY KEY (uid)
);