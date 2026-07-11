<?php
$submitted = !empty($_POST);

//Variables from Request access
$firstname = $_POST['firstname'];
$lastname = $_POST['lastname'];
$email = $_POST['email'];
$url   = $_POST['url'];
$birthday = $_POST['birthday']; 
$prof_or_student = $_POST['prof_or_student']; 
$drive_car = $_POST['drive_car']; 


?>

<!DOCTYPE html>
<html>
<!-- !--==== HTML SCRIPT ===== -->
<html lang = "en">
        <!-- === Head section === -->
    <head>
        <!-- === Include Special Character, Get out of ASCII default -->
        <meta charset="UTF-8">
        <!-- === Title ==== -->
        <title id="title">Request Access Page PHP</title>
        <meta name="viewport" content="width=device-width, initial-scale=1.0"> 
    </head>
    <!-- === Body section === -->
    <body>
        <!-- Begin PHP Echos  -->
        <p>Form Submitted? <?php echo (int) $submitted; ?> </p>
        <p>Your submitted information:</p>

        <!-- Begion Primary Listing items  -->
        <ul>
            <li><b>First Name:</b> <?php echo $firstname; ?></li>
            <li><b>Last Name:</b> <?php echo $lastname; ?></li>
            <li><b>Email:</b> <?php echo $email; ?></li>
            <li><b>URL:</b> <?php echo $url; ?></li>
            <li><b>Birthday:</b> <?php echo $birthday; ?></li>
            <li><b>Professor or Student:</b> <?php echo $prof_or_student; ?></li>
            <li><b>Do you drive?:</b> <?php echo $drive_car; ?></li>
            
            <!-- Secondary info -->
            <li>
                other information:
                <ul>
                    <!-- Loop Trhough the Involvement  -->
                    <?php 
                    foreach ($_POST['involvement'] as $involvemenet){
                        echo '<li>';
                        echo $involvemenet; 
                        echo '</li>'; 
                    }
                    ?>
                </ul>

            </li>

        </ul>
    </body>
</html>