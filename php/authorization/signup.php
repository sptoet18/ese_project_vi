<?php
	require_once __DIR__ . '/../util.php';

    session_start();

    $db = dbConnect(path, user, password);

	$username = $_POST['username'];
    $password = $_POST['password'];
    $firstname = $_POST['firstname'];
    $lastname = $_POST['lastname'];
    $role = $_POST['role'];

	$_SESSION['username'] = $username;

	$userInsert = $db->prepare('
        insert into users (
            username,
            password,
            firstname,
            lastname,
            role
        ) values (
            :username,
            :password,
            :firstname,
            :lastname,
            :role
        )
	');
	$userInsert->execute([
        'username'  => $username,
        'password'  => $password,
        'firstname' => $firstname,
        'lastname'  => $lastname,
        'role'      => $role
    ]);

    echo "<script>location.href = \"/php/authorization/member.php\"</script>";
?>