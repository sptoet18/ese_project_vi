<?php
	require_once __DIR__ . '/../util.php';

    session_start();

    //$db = dbConnect('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE');

	$username = $_POST['username'];
    $password = $_POST['password'];
    $firstname = $_POST['firstname'];
    $lastname = $_POST['lastname'];
    $role = $_POST['role'];

	$_SESSION['username'] = $username;

    insert_usr('mysql:host=127.0.0.1; dbname=elevator', 'Emiliano', 'ESE', $username, $password, $firstname, $lastname, $role);
   
    

	// $userInsert = $db->prepare('
    //     insert into user (
    //         username,
    //         hashed_password,
    //         firstname,
    //         lastname,
    //         role
    //     ) values (
    //         :username,
    //         :password,
    //         :firstname,
    //         :lastname,
    //         :role
    //     )
	// ');
	// $userInsert->execute([
    //     'username'  => $username,
    //     'password'  =>  $password,//password_hash($password, PASSWORD_DEFAULT),
    //     'firstname' => $firstname,
    //     'lastname'  => $lastname,
    //     'role'      => 'admin'
    // ]);

    echo "<script>location.href = \"/php/authorization/member.php\"</script>";
?>