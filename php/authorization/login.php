<?php
	require_once __DIR__ . '/../util.php';

    session_start();

    $db = dbConnect(path, user, password);

	$username = $_POST['username'];
    $password = $_POST['password'];

	$_SESSION['username'] = $username;

	$userQuery = $db->prepare('
		select id, username, hashed_password
		from users
		where username = :username
	');
	$userQuery->execute(['username' => $username]);
	$user = $userQuery->fetch();

    if ($user) {
		isCorrectPassword($password, $user['hashed_password']);
		echo "<script>location.href = \"/php/authorization/member.php\"</script>";
	} else {
		echo "<script>location.href = \"/html/authorization/login.html\"</script>";
	}
?>
