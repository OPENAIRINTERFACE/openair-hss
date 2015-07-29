-- phpMyAdmin SQL Dump
-- version 4.0.10deb1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Jul 29, 2015 at 05:52 PM
-- Server version: 5.5.44-0ubuntu0.14.04.1
-- PHP Version: 5.5.9-1ubuntu4.11

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
(1, 'ng40-erc.openair4G.eur', 'openair4G.eur', 0),
(3, 'ABEILLE.openair4G.eur', 'openair4G.eur', 0);

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
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=44 ;

--
-- Dumping data for table `pdn`
--

INSERT INTO `pdn` (`id`, `apn`, `pdn_type`, `pdn_ipv4`, `pdn_ipv6`, `aggregate_ambr_ul`, `aggregate_ambr_dl`, `pgw_id`, `users_imsi`, `qci`, `priority_level`, `pre_emp_cap`, `pre_emp_vul`, `LIPA-Permissions`) VALUES
(1, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000001', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(41, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '20834123456789', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(40, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '20810000001234', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(42, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '31002890832150', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(16, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000002', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(43, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '001010123456789', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(2, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000002', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(3, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000003', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(4, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000004', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(5, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000005', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(6, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000006', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(7, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930000000007', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(8, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000001', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(9, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000002', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(10, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000003', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(11, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000004', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(12, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000005', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(13, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000006', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(14, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208940000000007', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(15, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000001', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(17, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000003', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(18, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000004', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(19, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000005', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(20, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000006', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(21, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208950000000007', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(22, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001100', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(23, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001101', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(24, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001102', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(25, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001103', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(26, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001104', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(27, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001105', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(28, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001106', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(29, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001107', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(30, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001108', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(31, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001109', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(32, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208920100001110', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(33, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930100001111', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only'),
(34, 'oai.ipv4', 'IPv4', '0.0.0.0', '0:0:0:0:0:0:0:0', 50000000, 100000000, 3, '208930100001112', 9, 15, 'DISABLED', 'ENABLED', 'LIPA-only');

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
('20834123456789', '380561234567', '35609204079300', NULL, 'PURGED', 50, 40000000, 100000000, 47, 0000000000, 2, '+÷EüÇ≈≥\0ï,IHÅˇH', 0, 0, 00000000000000000096, 'PxºX \Z1°…xôﬂ', 'gÌêêjS+AqÑëß6Y'),
('20810000001234', '33611123456', '35609204079299', NULL, 'PURGED', 120, 40000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000281454575616225, '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0', 'é''∂Øi.u2fz;`]'),
('31002890832150', '33638060059', '35611302209414', NULL, 'PURGED', 120, 40000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012416, '`œFÆ›ÜÙÈD¢ºœõâ¡º', 'é''∂Øi.u2fz;`]'),
('001010123456789', '33600101789', '35609204079298', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, '\0	\n\r', 1, 0, 00000000000000000351, '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0', '$¿_|/+6ç·%/%ˆœ¬'),
('208930000000001', '33638030001', '35609204079301', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208950000000002', '33638050002', '35609204079502', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 3, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012215, 'V&ùcQ[ö&†\\Ï', 'é''∂Øi.u2fz;`]'),
('208950000000003', '33638050003', '35609204079503', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 3, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012215, '56f0261d9d051063', 'é''∂Øi.u2fz;`]'),
('208950000000004', '33638050004', '35609204079504', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 3, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012215, '56f0261d9d051063', 'é''∂Øi.u2fz;`]'),
('208950000000005', '33638050005', '35609204079505', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 3, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012215, '56f0261d9d051063', 'é''∂Øi.u2fz;`]'),
('208950000000001', '33638050001', '35609204079501', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208950000000006', '33638050006', '35609204079506', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 3, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012215, '56f0261d9d051063', 'é''∂Øi.u2fz;`]'),
('208950000000007', '33638050007', '35609204079507', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 3, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000012215, '56f0261d9d051063', 'é''∂Øi.u2fz;`]'),
('208930000000002', '33638030002', '35609204079302', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208930000000003', '33638030003', '35609204079303', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208930000000004', '33638030004', '35609204079304', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208930000000005', '33638030005', '35609204079305', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208930000000006', '33638030006', '35609204079306', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208930000000007', '33638030007', '35609204079307', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000007', '33638040007', '35609204079407', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000006', '33638040006', '35609204079406', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000005', '33638040005', '35609204079405', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000004', '33638040004', '35609204079404', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000003', '33638040003', '35609204079403', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000002', '33638040002', '35609204079402', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208940000000001', '33638040001', '35609204079401', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'Î–wq¨ËgzWÆ–Å≤·Z]', 'é''∂Øi.u2fz;`]'),
('208920100001100', '33638020001', '35609204079201', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001101', '33638020001', '35609204079201', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001102', '33638020002', '35609204079202', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001103', '33638020003', '35609204079203', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001104', '33638020004', '35609204079204', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001105', '33638020005', '35609204079205', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001106', '33638020006', '35609204079206', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001107', '33638020007', '35609204079207', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001108', '33638020008', '35609204079208', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001109', '33638020009', '35609204079209', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208920100001110', '33638020010', '35609204079210', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208930100001111', '33638030011', '35609304079211', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]'),
('208930100001112', '33638030012', '35609304079212', NULL, 'PURGED', 120, 50000000, 100000000, 47, 0000000000, 2, 'ãØG?/è–îáÃÀ◊	|hb', 1, 0, 00000000000000006103, 'ebd07771ace8677a', 'é''∂Øi.u2fz;`]');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
