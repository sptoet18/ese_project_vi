<!--AUTHOR: Emiliano Perez Pellicer
    DATE: 22-05-2026
    COMMENTS: This will be my Main Page in which you can acces my Logbooks and Info page 
    VERSION: 1.0 
-->
<?php
// Add a submitted variable that check the type of submission 
$submitted = !empty($_POST);
$username = $_POST['username'];
$password = $_POST['password'];

//Print the Submition details 
echo "<p>Form Submitted succesfully! (1 for true): $submitted </p>"; 
echo "<p>Username recieved: $username </p>"; 
echo "<p>Password recieved: $password </p>"; 

echo "<p>Copyright &copy Emiliano Perez Pellicer</p>"; 
?>
