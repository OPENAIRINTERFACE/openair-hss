-- phpMyAdmin SQL Dump
-- version 4.0.10deb1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: May 28, 2015 at 02:32 PM
-- Server version: 5.5.43-0ubuntu0.14.04.1
-- PHP Version: 5.5.9-1ubuntu4.9

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `oai_db`
--

-- --------------------------------------------------------

--
-- Table structure for table `apn`
--

CREATE TABLE IF NOT EXISTS `apn` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `apn-name` varchar(60) NOT NULL,
  `pdn-type` enum('IPv4','IPv6','IPv4v6','IPv4_or_IPv6') NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `apn-name` (`apn-name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `mmeidentity`
--

CREATE TABLE IF NOT EXISTS `mmeidentity` (
  `idmmeidentity` int(11) NOT NULL AUTO_INCREMENT,
  `mmehost` varchar(255) DEFAULT NULL,
  `mmerealm` varchar(200) DEFAULT NULL,
  `UE-Reachability` tinyint(1) NOT NULL COMMENT 'Indicates whether the MME supports UE Reachability Notifcation',
  PRIMARY KEY (`idmmeidentity`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=43 ;

--
-- Dumping data for table `mmeidentity`
--

INSERT INTO `mmeidentity` (`idmmeidentity`, `mmehost`, `mmerealm`, `UE-Reachability`) VALUES
(2, 'yang.openair4G.eur', 'openair4G.eur', 0),
(1, 'ng40-erc.openair4G.eur', 'openair4G.eur', 0);

-- --------------------------------------------------------

--
-- Table structure for table `pdn`
--

CREATE TABLE IF NOT EXISTS `pdn` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `apn` varchar(60) NOT NULL,
  `pdn_type` enum('IPv4','IPv6','IPv4v6','IPv4_or_IPv6') NOT NULL DEFAULT 'IPv4',
  `pdn_ipv4` varchar(15) DEFAULT '0.0.0.0',
  `pdn_ipv6` varchar(45) CHARACTER SET latin1 COLLATE latin1_general_ci DEFAULT '0:0:0:0:0:0:0:0',
  `aggregate_ambr_ul` int(10) unsigned DEFAULT '50000000',
  `aggregate_ambr_dl` int(10) unsigned DEFAULT '100000000',
  `pgw_id` int(11) NOT NULL,
  `users_imsi` varchar(15) NOT NULL,
  `qci` tinyint(3) unsigned NOT NULL DEFAULT '9',
  `priority_level` tinyint(3) unsigned NOT NULL DEFAULT '15',
  `pre_emp_cap` enum('ENABLED','DISABLED') DEFAULT 'DISABLED',
  `pre_emp_vul` enum('ENABLED','DISABLED') DEFAULT 'DISABLED',
  `LIPA-Permissions` enum('LIPA-prohibited','LIPA-only','LIPA-conditional') NOT NULL DEFAULT 'LIPA-only',
  PRIMARY KEY (`id`,`pgw_id`,`users_imsi`),
  KEY `fk_pdn_pgw1_idx` (`pgw_id`),
  KEY `fk_pdn_users1_idx` (`users_imsi`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=36 ;

--
-- Dumping data for table `pdn`
--

INSERT INTO `pdn` (`id`, `apn`, `pdn_type`, `pdn_ipv4`, `pdn_ipv6`, `aggregate_ambr_ul`, `aggregate_ambr_dl`, `pgw_id`, `users_imsi`, `qci`, `priority_level`, `pre_emp_cap`, `pre_emp_vul`, `LIPA-Permissions`) VALUES
(1, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000001', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(11, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '20834123456789', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(10, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '20810000001234', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(12, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '31002890832150', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(2, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000002', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(3, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '001010123456789', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only');

-- --------------------------------------------------------

--
-- Table structure for table `pgw`
--

CREATE TABLE IF NOT EXISTS `pgw` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ipv4` varchar(15) NOT NULL,
  `ipv6` varchar(39) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `ipv4` (`ipv4`),
  UNIQUE KEY `ipv6` (`ipv6`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=4 ;

--
-- Dumping data for table `pgw`
--

INSERT INTO `pgw` (`id`, `ipv4`, `ipv6`) VALUES
(1, '127.0.0.1', '0:0:0:0:0:0:0:1'),
(2, '192.168.56.101', ''),
(3, '10.0.0.2', '0');

-- --------------------------------------------------------

--
-- Table structure for table `terminal-info`
--

CREATE TABLE IF NOT EXISTS `terminal-info` (
  `imei` varchar(15) NOT NULL,
  `sv` varchar(2) NOT NULL,
  UNIQUE KEY `imei` (`imei`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `users`
--

CREATE TABLE IF NOT EXISTS `users` (
  `imsi` varchar(15) NOT NULL COMMENT 'IMSI is the main reference key.',
  `msisdn` varchar(46) DEFAULT NULL COMMENT 'The basic MSISDN of the UE (Presence of MSISDN is optional).',
  `imei` varchar(15) DEFAULT NULL COMMENT 'International Mobile Equipment Identity',
  `imei_sv` varchar(2) DEFAULT NULL COMMENT 'International Mobile Equipment Identity Software Version Number',
  `ms_ps_status` enum('PURGED','NOT_PURGED') DEFAULT 'PURGED' COMMENT 'Indicates that ESM and EMM status are purged from MME',
  `rau_tau_timer` int(10) unsigned DEFAULT '120',
  `ue_ambr_ul` bigint(20) unsigned DEFAULT '50000000' COMMENT 'The Maximum Aggregated uplink MBRs to be shared across all Non-GBR bearers according to the subscription of the user.',
  `ue_ambr_dl` bigint(20) unsigned DEFAULT '100000000' COMMENT 'The Maximum Aggregated downlink MBRs to be shared across all Non-GBR bearers according to the subscription of the user.',
  `access_restriction` int(10) unsigned DEFAULT '60' COMMENT 'Indicates the access restriction subscription information. 3GPP TS.29272 #7.3.31',
  `mme_cap` int(10) unsigned zerofill DEFAULT NULL COMMENT 'Indicates the capabilities of the MME with respect to core functionality e.g. regional access restrictions.',
  `mmeidentity_idmmeidentity` int(11) NOT NULL DEFAULT '0',
  `key` varbinary(16) NOT NULL DEFAULT '0' COMMENT 'UE security key',
  `RFSP-Index` smallint(5) unsigned NOT NULL DEFAULT '1' COMMENT 'An index to specific RRM configuration in the E-UTRAN. Possible values from 1 to 256',
  `urrp_mme` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'UE Reachability Request Parameter indicating that UE activity notification from MME has been requested by the HSS.',
  `sqn` bigint(20) unsigned zerofill NOT NULL,
  `rand` varbinary(16) NOT NULL,
  `OPc` varbinary(16) DEFAULT NULL COMMENT 'Can be computed by HSS',
  PRIMARY KEY (`imsi`,`mmeidentity_idmmeidentity`),
  KEY `fk_users_mmeidentity_idx1` (`mmeidentity_idmmeidentity`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `users`
--

INSERT INTO `users` (`imsi`, `msisdn`, `imei`, `imei_sv`, `ms_ps_status`, `rau_tau_timer`, `ue_ambr_ul`, `ue_ambr_dl`, `access_restriction`, `mme_cap`, `mmeidentity_idmmeidentity`, `key`, `RFSP-Index`, `urrp_mme`, `sqn`, `rand`, `OPc`) VALUES
('20834123456789', '380561234567', '12345678', '23', 'PURGED', 50, 40000000, 100000000, 47, 0000000000, 2, '+÷EüÇ≈≥\0ï,IHÅˇH', 0, 0, 00000000000000000096, 'PxºX \Z1°…xôﬂ', 'gÌêêjS+AqÑëß6Y'),
('20810000001234', '33611123456', NULL, NULL, 'PURGED', 120, 40000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000281454575616097, 'sÀ⁄Ê$rÆCπfâ=:æxﬁ', 'é''∂Øi.u2fz;`]'),
('31002890832150', '33638060059', '35611302209414', NULL, 'PURGED', 120, 40000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012416, '`œFÆ›ÜÙÈD¢ºœõâ¡º', 'é''∂Øi.u2fz;`]'),
('001010123456789', '33600101789', NULL, NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, '\0	\n\r', 1, 0, 00000000000000000351, '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0', '$¿_|/+6ç·%/%ˆœ¬'),
('208930000000001', '33638060008', NULL, NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208950000000002', '33638060009', NULL, NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006391, 'U∂|« w±DÃ¸\ZÖ7', 'é''∂Øi.u2fz;`]');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
